#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_dev.h>
#include <rte_bus.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_alarm.h>
#include <rte_vhost.h>

#include "vhost.h"

static uint16_t get_ip_len(struct rte_mbuf *buf)
{
	struct rte_mbuf *pkt;
	struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	uint16_t ether_type;
	uint16_t ip_len;

	pkt = buf;
	eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
	ether_type = eth_hdr->ether_type;

	if(!MATCH_TYPE(ether_type, SD_ETHER_TYPE_IPV4))
		return 0;
	ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
	ip_len = ntohs(ipv4_hdr->total_length);
	return ip_len;
}

// 接收数据包，放入fwd_ring
static int recv_pkt(void *arg)
{
	uint16_t ip_len;
	uint16_t port = *( (uint16_t *)arg );
	struct rte_mbuf *bufs[BURST_SIZE];
	int nb_rx = 0;

	/**
	 * 等待rte_eth_dev_callback_register注册的回调函数执行
	 * 当virtio端设置完成，vhost驱动通过vring_state_changed函数调用注册的回调函数
	 * 回调函数中将fore_quit设置为false
	 */
	while(force_quit);

	printf("Start Receiving...\n");
	while(!force_quit) {
		nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

		if (unlikely(nb_rx == 0))
			continue;
		statistics.total_rx_num += nb_rx;

		for (int i = 0; i < nb_rx; i++) {
			if (rte_ring_enqueue(fwd_ring, bufs[i]) < 0) {
                rte_pktmbuf_free(bufs[i]);
				continue;
            }
			statistics.real_rx_num += 1;

			ip_len = get_ip_len(bufs[i]);
			statistics.rx_bytes += (uint64_t)ip_len + 14;
		}
	}

	return 0;
}



// 从fwd_ring取出数据包，发送至virtio端
static int send_pkt(void *arg)
{
	uint16_t port = *( (uint16_t *)arg );
	struct rte_mbuf *bufs[TX_SIZE];
	struct rte_mbuf *tmp;
	int burst = 0;
	int retval = 0;
	uint16_t nb_tx = 0;

	uint16_t ip_len;

	/**
	 * 等待rte_eth_dev_callback_register注册的回调函数执行
	 * 当virtio端设置完成，vhost驱动通过vring_state_changed函数调用注册的回调函数(virtio_ready)
	 * 回调函数中将fore_quit设置为false
	 */
	while(force_quit);

	printf("Start Sending...\n");
	while(!force_quit) {
		burst = 0;
		retval = rte_ring_dequeue(fwd_ring, (void **)&tmp);
		while(burst < TX_SIZE && retval == 0) {
			bufs[burst++] = tmp;
			retval = rte_ring_dequeue(fwd_ring, (void **)&tmp);
		}
		if(!burst)
			continue;

		statistics.total_tx_num += burst;
		for(int i = 0; i < burst; i++) {
			ip_len = get_ip_len(bufs[i]);
			statistics.tx_bytes += (uint64_t)ip_len + 14;
		}
		
		nb_tx = rte_eth_tx_burst(port, 0, bufs, burst);
		statistics.real_tx_num += nb_tx;
		if (unlikely(nb_tx < burst)) {
			for (uint16_t i = nb_tx; i < burst; i++) {
				ip_len = get_ip_len(bufs[i]);
				statistics.tx_bytes -= (uint64_t)ip_len + 14;
				rte_pktmbuf_free(bufs[i]);
			}
				
		}
	}

	while(rte_ring_dequeue(fwd_ring, (void **)&tmp) == 0)
		rte_pktmbuf_free(tmp);

	return 0;
}

// 信号处理函数
static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

// virtio端就绪回调函数
static int virtio_ready(uint16_t port_id,
		__rte_unused enum rte_eth_event_type type,
		__rte_unused void *param,
		__rte_unused void *ret_param)
{
	struct rte_eth_vhost_queue_event event;
	rte_eth_vhost_get_queue_event(port_id, &event);
	if(event.enable) {
		force_quit = false;
		clock_gettime(CLOCK_MONOTONIC, &statistics.start_time);
		printf("Virtio Ready for Receiving Packet\n");
	}
	return 0;
}

int main(int argc, char *argv[]) {
	int ret;
    unsigned int nb_ports;
    unsigned int nb_lcores;
    uint16_t portid;

    struct rte_mempool *mbuf_pool;

    uint16_t recv_port = 0;
	uint16_t send_port = 1;

	force_quit = true;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	if(generate_logdir(LOG_DIR) != 0)
		return -1;
    
    // EAL环境初始化
    ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    
    // 工作核心数量
	nb_lcores = rte_lcore_count();
	if (nb_lcores < 1)
		rte_exit(EXIT_FAILURE, "Error: no enough lcores\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("Ports Number: %u\n", nb_ports);

    // 分配内存池
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // 初始化网卡
	RTE_ETH_FOREACH_DEV(portid) {
        ret = port_init(portid, mbuf_pool);
        if(ret < 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", portid);
        if((PORT_TYPE)ret == SEND_PORT) {
			send_port = (uint16_t)portid;
			rte_eth_dev_callback_register(portid, RTE_ETH_EVENT_QUEUE_STATE, virtio_ready, NULL);
		}
        else
            recv_port = (uint16_t)portid;
    }		
    
    // 共享队列初始化
	ring_init(&fwd_ring, "fwd_ring");

    printf("\nStart Processing...\n\n");

    // 分配工作核心任务
    unsigned int worker_id = -1;
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(send_pkt, &send_port, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(recv_pkt, &recv_port, worker_id);
	

	FILE *logfile = fopen(LOG_FILE, "w");
	rte_eal_alarm_callback cb = period_show_stats;
	statistics.log = logfile;
	// rte_eal_alarm_set(PERIOD * MS_PER_S, cb, &statistics);

	// 等待工作核心结束任务
	rte_eal_mp_wait_lcore();

	show_results(&statistics);

	// 关闭网卡
	RTE_ETH_FOREACH_DEV(portid) {
		printf("Closing port %d...\n", portid);
		ret = rte_eth_dev_stop(portid);
		if (ret != 0)
			printf("rte_eth_dev_stop: err=%d, port=%d\n",
			       ret, portid);
		rte_eth_dev_close(portid);
		printf("Done\n");
	}
    
	rte_eth_dev_callback_unregister(portid, RTE_ETH_EVENT_QUEUE_STATE, virtio_ready, NULL);
	// rte_eal_alarm_cancel(cb, &statistics);
	fclose(logfile);
	rte_ring_free(fwd_ring);

    rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}