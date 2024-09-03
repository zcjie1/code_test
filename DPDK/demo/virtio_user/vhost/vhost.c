/**
 * vhost驱动程序测试
 * 1. 获取运行参数中的-vdev参数，并加载对应驱动
 * 2. 加载一张物理网卡，一张vhost网卡
 * 3. 通过物理网卡接收字符串数据，并通过vhost网卡转发
 * 4. virtio端接收vhost网卡转发字符串，并输出至文件(挂载到容器中)
 * 
 * 运行命令: sudo ./build/vhost -l 0-1 --vdev 'eth_vhost0,iface=/tmp/sock0'
 */

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
#include <rte_vhost.h>

#define NUM_MBUFS 4096
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 32

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 2048
#define TX_RING_SIZE 2048

#define PERIOD 10

bool force_quit = false;

struct status {
	uint64_t total_rx;
	uint64_t total_tx;
	uint64_t real_rx;
	uint64_t real_tx;
}statistics;

static struct rte_ring *fwd_ring;

typedef enum PORT_TYPE {
    RECV_PORT = 0,
    SEND_PORT,
}PORT_TYPE;

// 接收数据包，放入fwd_ring
static int recv_pkt(void *arg)
{
	uint16_t port = *( (uint16_t *)arg );
	struct rte_mbuf *bufs[BURST_SIZE];
	int nb_rx = 0;

	while(!force_quit) {
		nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

		if (unlikely(nb_rx == 0))
			continue;

		statistics.total_rx += nb_rx;

		for (int i = 0; i < nb_rx; i++) {
			if (rte_ring_enqueue(fwd_ring, bufs[i]) < 0) {
                rte_pktmbuf_free(bufs[i]);
				continue;
            }
			statistics.real_rx += 1;
		}
	}
	return 0;
}

// 从fwd_ring取出数据包，发送至virtio端
static int send_pkt(void *arg)
{
	uint16_t port = *( (uint16_t *)arg );
	struct rte_mbuf *bufs[BURST_SIZE];
	struct rte_mbuf *tmp;
	int burst = 0;
	int retval = 0;
	uint16_t nb_tx = 0;

	struct rte_ether_addr src_mac;
	rte_eth_macaddr_get(port, &src_mac);

	while(!force_quit) {
		burst = 0;
		retval = rte_ring_dequeue(fwd_ring, (void **)&tmp);
		while(burst < BURST_SIZE && retval == 0) {
			bufs[burst++] = tmp;
			retval = rte_ring_dequeue(fwd_ring, (void **)&tmp);
		}

		statistics.total_tx += burst;

		nb_tx = rte_eth_tx_burst(port, 0, bufs, burst);
		statistics.real_tx += nb_tx;
		if (unlikely(nb_tx < burst)) {
			for (uint16_t i = nb_tx; i < burst; i++)
				rte_pktmbuf_free(bufs[i]);
		}
	}

	while(rte_ring_dequeue(fwd_ring, (void **)&tmp) == 0)
		rte_pktmbuf_free(tmp);

	return 0;
}

// 共享队列初始化
static void ring_init()
{
	fwd_ring = rte_ring_create("rx_ring", RX_RING_SIZE, 
					rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
}

// 网卡初始化
static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf;
	struct rte_eth_dev_info dev_info;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
    PORT_TYPE type = RECV_PORT;

	struct rte_eth_rxconf rxconf;
	struct rte_eth_txconf txconf;
	
	printf("Initializing port %u... \n", port);
	fflush(stdout);

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	// 获取网卡信息
	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval < 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

	// 配置port_conf
	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

	// 设置网卡对应的ring数量
	retval = rte_eth_dev_configure(port, RX_RING_NUM, TX_RING_NUM, &port_conf);
	if (retval < 0)
		return retval;

	// 设置每个ring的描述符数量
	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval < 0)
		return retval;

	// 设置每个receive ring对应的内存池
	int port_socket = rte_eth_dev_socket_id(port);
	rxconf = dev_info.default_rxconf;
	rxconf.offloads = port_conf.rxmode.offloads;
	for (int r = 0; r < RX_RING_NUM; r++) {
		retval = rte_eth_rx_queue_setup(port, r, nb_rxd,
				port_socket, &rxconf, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	// 配置发送队列
	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	for (int r = 0; r < TX_RING_NUM; r++) {
		retval = rte_eth_tx_queue_setup(port, r, nb_txd,
				port_socket, &txconf);
		if (retval < 0)
			return retval;
	}

	retval = rte_eth_dev_set_ptypes(port, RTE_PTYPE_UNKNOWN, NULL, 0);
	if (retval < 0)
		printf("Port %u, Failed to disable Ptype parsing\n", port);

	// 启动网卡
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	// 输出网卡信息
	struct rte_ether_addr addr;
    char *name = (char *)malloc(sizeof(char) * 512);
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval < 0)
		return retval;

	printf("Port %u: \n", port);
    printf("    MAC: %02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 ":"
			   "%02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 "\n",
			RTE_ETHER_ADDR_BYTES(&addr));
    
    rte_eth_dev_get_name_by_port(port, name);
    printf("    Device Name: %s\n", name);
    printf("    Driver Name: %s\n\n", dev_info.driver_name);

    free(name);

    if(strcmp(dev_info.driver_name, "net_vhost") == 0)
        type = SEND_PORT;

	retval = rte_eth_promiscuous_enable(port);
	if (retval < 0)
		return retval;

	return type;
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

static void show_stats(void *param)
{
	FILE *log = (FILE *)param;
	time_t now;
    struct tm *local_time;
    char timestamp[26];
	uint64_t total = statistics.total_rx + statistics.total_tx;
	uint64_t real = statistics.real_rx + statistics.real_tx;
	uint64_t drop = total - real;

	// 获取当前时间
    time(&now);
    local_time = localtime(&now);

	// 格式化时间戳
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

	fprintf(log, "\n==================Packets statistics=======================");
	fprintf(log, "\nPackets sent: %24"PRIu64
			"\nPackets received: %20"PRIu64
			"\nPackets dropped: %21"PRIu64,
			statistics.real_tx,
			statistics.real_rx,
			drop);
	fprintf(log, "\n===================%s=====================\n\n", timestamp);
	fflush(stdout);

	rte_eal_alarm_set(PERIOD * US_PER_S, show_stats, log);
}

int main(int argc, char *argv[]) {

    unsigned int nb_ports;
    unsigned int nb_lcores;
    uint16_t portid;

    struct rte_mempool *mbuf_pool;

    uint16_t recv_port = 0;
	uint16_t send_port = 1;

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
        if((PORT_TYPE)ret == SEND_PORT)
            send_port = (uint16_t)portid;
        else
            recv_port = (uint16_t)portid;
    }		
    
    // 共享队列初始化
	ring_init();

    printf("\nStart Processing...\n\n");

    // 分配工作核心任务
    unsigned int worker_id = -1;
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(recv_pkt, &recv_port, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(send_pkt, &send_port, worker_id);

	FILE *logfile = fopen("packet.log", "w");
	rte_eal_alarm_callback cb = show_stats;
	rte_eal_alarm_set(PERIOD * MS_PER_S, cb, logfile);

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
    
	rte_eal_alarm_cancel(cb, logfile);
	fclose(logfile);
	rte_ring_free(fwd_ring);

    rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}