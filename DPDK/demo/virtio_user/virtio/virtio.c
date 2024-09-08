#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
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

#include "virtio.h"

struct timespec start_time;
struct statistic result;

static bool same_mac(struct rte_ether_addr *a, struct rte_ether_addr *b)
{
	for(int i = 0; i < RTE_ETHER_ADDR_LEN; i++) {
		if(a->addr_bytes[i] != b->addr_bytes[i])
			return false;
	}

	return true;
}

// 输出数据包的元数据和字符串消息至挂载文件
static int process_pkt(__rte_unused void *arg)
{
	FILE *statistic_log = fopen(LOG_STATISTIC, "w");
	FILE *packet_log = fopen(LOG_PACKET, "w");

	struct rte_mbuf *pkt;
	struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	struct rte_ether_addr src_mac, dst_mac;
	uint16_t ether_type;
	uint32_t src_ip, dst_ip;
	uint16_t ip_len;
    void *data;
	int ret;

    char *msg = (char *)malloc(sizeof(char) * 256);
	msg[255] = '\0';

    struct rte_mbuf *bufs[BURST_SIZE];
	int nb_rx = 0;

	curr_time(&start_time);

	while (!force_quit) {
		nb_rx = rte_eth_rx_burst(0, 0, bufs, BURST_SIZE);
		if (unlikely(nb_rx == 0))
			continue;
		
        result.rx_num += nb_rx;
        for(int i = 0; i < nb_rx; i++) {
            pkt = bufs[i];

            // 解析二层网络
            eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
            src_mac = eth_hdr->src_addr;
            dst_mac = eth_hdr->dst_addr;
            ether_type = eth_hdr->ether_type;

            if(!MATCH_TYPE(ether_type, SD_ETHER_TYPE_IPV4)) {
				// show_packet(packet_log, src_mac, dst_mac, ether_type, 0, 0, NULL);
                rte_pktmbuf_free(pkt);
                continue;
            }

            // 解析三层网络
            ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
            src_ip = ipv4_hdr->src_addr;
            dst_ip = ipv4_hdr->dst_addr;
			ip_len = ntohs(ipv4_hdr->total_length);

			result.rx_bytes += (uint64_t)ip_len + 14;

			if(same_mac(&dst_mac, &broadcast_mac)) {
				data = (void *)(ipv4_hdr + 1);
				memccpy(msg, data, '\0', 255);
				show_packet(packet_log, src_mac, dst_mac, ether_type, src_ip, dst_ip, msg);
			}

            // 解析字符串消息
            // data = (void *)(ipv4_hdr + 1);
            // memccpy(msg, data, '\0', 255);
            // show_packet(packet_log, src_mac, dst_mac, ether_type, src_ip, dst_ip, msg);

            rte_pktmbuf_free(pkt);
        }
	}

	show_result(statistic_log);

	fclose(statistic_log);
	fclose(packet_log);
	free(msg);
	return 0;
        
}

static int send_pkt(void *arg)
{
	const struct rte_ether_addr src_mac = {
		.addr_bytes = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc }
	};
	const struct rte_ether_addr dst_mac = { 
		.addr_bytes = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } 
	};
	char sip[] = "192.0.2.1";
	char dip[] = "192.0.2.254";
	struct rte_mempool *mbuf_pool = (struct rte_mempool *)arg;

	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);
	if(!pkt) {
		printf("Failed to allocate mbuf\n");
		return -1;
	}

	// Meta data 初始化
	pkt->data_len = 14 + 20 + 15; // MAC + IPv4 + Message
	pkt->data_off = RTE_PKTMBUF_HEADROOM;
	pkt->pkt_len = 14 + 20 + 15;
	pkt->nb_segs = 1;
	// pkt->ol_flags &= RTE_MBUF_F_EXTERNAL;
	pkt->l2_len	= sizeof(struct rte_ether_hdr);
	pkt->l3_len	= sizeof(struct rte_ipv4_hdr);
	pkt->next = NULL;

	// printf("Meta data initialized\n");

	// 二层初始化
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
	rte_ether_addr_copy(&src_mac, &eth_hdr->src_addr);
	rte_ether_addr_copy(&dst_mac, &eth_hdr->dst_addr);
	eth_hdr->ether_type = SD_ETHER_TYPE_IPV4;

	// printf("L2 initialized\n");

	// 三层初始化
	struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
	memset(ipv4_hdr, 0, sizeof(struct rte_ipv4_hdr));
	ipv4_hdr->version_ihl = RTE_IPV4_VHL_DEF;
	ipv4_hdr->type_of_service = 0;
	ipv4_hdr->total_length = htons((uint16_t)35);
	ipv4_hdr->packet_id = 0;
	ipv4_hdr->fragment_offset = 0;
	ipv4_hdr->time_to_live = 64;
	ipv4_hdr->next_proto_id = IPPROTO_RAW;
	ipv4_hdr->src_addr = inet_addr(sip);
	ipv4_hdr->dst_addr = inet_addr(dip);
	ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
	char *data = (char *)(ipv4_hdr + 1);
	memcpy(data, "Hello, world!\0", 15);

	rte_mbuf_refcnt_set(pkt, 1);

	while(!force_quit) {
		rte_delay_ms(1000);
		rte_mbuf_refcnt_update(pkt, 1);
		int nb_tx = rte_eth_tx_burst(0, 0, &pkt, 1);
		if(nb_tx < 1) {
			rte_pktmbuf_free(pkt);
			printf("Failed to transmit all packets\n");
		} else {
			printf("Send %d packet\n", nb_tx);
		}
	}

	rte_pktmbuf_free(pkt);
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

int main(int argc, char *argv[]) {
    unsigned int nb_ports;
    unsigned int nb_lcores;
    uint16_t portid;

    struct rte_mempool *mbuf_pool;

    force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
    
    // EAL环境初始化
    int ret = rte_eal_init(argc, argv);
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
    }

	printf("\nStart Processing...\n\n");

    // 分配工作核心任务
    unsigned int worker_id = -1;
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(process_pkt, NULL, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(send_pkt, mbuf_pool, worker_id);

	// printf("\nStarting loop alarm...\n\n");
	// rte_eal_alarm_callback cb = period_alarm_vhost;
	// rte_eal_alarm_set(PERIOD * MS_PER_S, cb, mbuf_pool);

    // 等待工作核心结束任务
	rte_eal_mp_wait_lcore();

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

	rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}