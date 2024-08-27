/* 
1. scapy发送IP数据包（包含字符串消息）
2. DPDK: 
	工作核心1接收IP数据包
    工作核心2打印数据包的五元组和字符串消息并储存统计信息
    工作核心3广播该数据包
    主核心读取统计信息并定时打印 
*/

#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_alarm.h>

#define NUM_MBUFS 4096
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 32

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define PERIOD 5

struct status {
	uint64_t total_rx;
	uint64_t total_tx;
	uint64_t real_rx;
	uint64_t real_tx;
}statistics;

// 工作核心共享队列
static struct rte_ring *rx_ring;
static struct rte_ring *tx_ring;

// 强制退出标志位
volatile bool force_quit;

// 接收IP数据包，并放入rx_ring
static int recv_pkt(void *arg)
{
	uint16_t port = *( (uint16_t *)arg );
	struct rte_mbuf *bufs[BURST_SIZE];
	int nb_rx = 0;

	// printf("start receive packet.\n");

	while(!force_quit) {
		nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

		if (unlikely(nb_rx == 0))
			continue;

		statistics.total_rx += nb_rx;

		for (int i = 0; i < nb_rx; i++) {
			if (rte_ring_enqueue(rx_ring, bufs[i]) < 0) {
                rte_pktmbuf_free(bufs[i]);
				continue;
            }
			statistics.real_rx += 1;
		}
	}
	return 0;
}

static void print_addr(struct rte_ether_addr src_mac, 
				struct rte_ether_addr dst_mac, 
				uint32_t src_ip, uint32_t dst_ip)
{
	char smac[RTE_ETHER_ADDR_FMT_SIZE];
	char dmac[RTE_ETHER_ADDR_FMT_SIZE];
	char sip[INET_ADDRSTRLEN];
	char dip[INET_ADDRSTRLEN];

	rte_ether_format_addr(smac, RTE_ETHER_ADDR_FMT_SIZE, &src_mac);
	rte_ether_format_addr(dmac, RTE_ETHER_ADDR_FMT_SIZE, &dst_mac);

	inet_ntop(AF_INET, &src_ip, sip, sizeof(sip));
	inet_ntop(AF_INET, &dst_ip, dip, sizeof(dip));

	printf("[ %s | %s ] -> [ %s | %s ]: ", sip, smac, dip, dmac);
}

static bool is_same_mac(struct rte_ether_addr *a, struct rte_ether_addr *b)
{
	for(int i = 0; i < RTE_ETHER_ADDR_LEN; i++) {
		if(a->addr_bytes[i] != b->addr_bytes[i])
			return false;
	}

	return true;
}

// 输出数据包的元数据和字符串消息
static int parse_pkt(__rte_unused void *arg)
{
	struct rte_mbuf *pkt;
	struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	void *data;
	struct rte_ether_addr src_mac, dst_mac;
	uint16_t ether_type;
	uint32_t src_ip, dst_ip;
	char *msg = (char *)malloc(sizeof(char) * 256);
	msg[255] = '\0';
	int retval;

	struct rte_ether_addr send_mac = {
        .addr_bytes = { 0x11, 0x11, 0x11, 0x22, 0x22, 0x22 }
    };

	// printf("start parse packet.\n");

	while (!force_quit) {
		retval = rte_ring_dequeue(rx_ring, (void **)&pkt);
		if(retval < 0) {
			usleep(1000);
			continue;
		}

		// 解析二层网络
		eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
		src_mac = eth_hdr->src_addr;
		dst_mac = eth_hdr->dst_addr;
		ether_type = eth_hdr->ether_type;

		// printf("origin: %02x:%02x:%02x:%02x:%02x:%02x\n", 
		// 	src_mac.addr_bytes[0], src_mac.addr_bytes[1], src_mac.addr_bytes[2], 
		// 	src_mac.addr_bytes[3], src_mac.addr_bytes[4], src_mac.addr_bytes[5]
		// );

		if(ether_type != (uint16_t)0x0008 || !is_same_mac(&src_mac, &send_mac)) {
			rte_pktmbuf_free(pkt);
			continue;
		}

		// 解析三层网络
		ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
		src_ip = ipv4_hdr->src_addr;
		dst_ip = ipv4_hdr->dst_addr;

		// 解析字符串消息
		data = (void *)(ipv4_hdr + 1);
		memccpy(msg, data, '\0', 255);
		print_addr(src_mac, dst_mac, src_ip, dst_ip);
		printf("%s\n", msg);

		if(rte_ring_enqueue(tx_ring, pkt) < 0) {
			rte_pktmbuf_free(pkt);
			printf("Error: fail to send one packet.\n");
		}
	}
	while(rte_ring_dequeue(rx_ring, (void **)&pkt) == 0)
		rte_pktmbuf_free(pkt);
	free(msg);
	return 0;
        
}

// 更新数据包源地址和目的地址
static void update_pkt(struct rte_mbuf **buf_table, int size, 
				struct rte_ether_addr src_mac)
{
	struct rte_mbuf *pkt;
	struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	uint32_t src_ip, dst_ip;

	struct rte_ether_addr broadcast_mac = {
        .addr_bytes = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
    };

	for(int i = 0; i < size; i++) {
		pkt = buf_table[i];

		// 解析二层网络
		eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

		// 解析三层网络
		ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
		src_ip = ipv4_hdr->src_addr;
		dst_ip = ipv4_hdr->dst_addr;

		ipv4_hdr->src_addr = dst_ip;
		ipv4_hdr->dst_addr = src_ip;

		eth_hdr->src_addr = src_mac;
		eth_hdr->dst_addr = broadcast_mac;
	}
}

// 转发数据包
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

	// printf("start send packet.\n");

	while(!force_quit) {
		burst = 0;
		retval = rte_ring_dequeue(tx_ring, (void **)&tmp);
		while(burst < BURST_SIZE && retval == 0) {
			bufs[burst++] = tmp;
			retval = rte_ring_dequeue(tx_ring, (void **)&tmp);
		}

		update_pkt(bufs, burst, src_mac);

		statistics.total_tx += burst;

		nb_tx = rte_eth_tx_burst(port, 0, bufs, burst);
		statistics.real_tx += nb_tx;
		if (unlikely(nb_tx < burst)) {
			for (uint16_t i = nb_tx; i < burst; i++)
				rte_pktmbuf_free(bufs[i]);
		}
	}
	while(rte_ring_dequeue(tx_ring, (void **)&tmp) == 0)
		rte_pktmbuf_free(tmp);

	return 0;
}

// 网卡初始化
static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf;
	struct rte_eth_dev_info dev_info;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;

	struct rte_eth_rxconf rxconf;
	struct rte_eth_txconf txconf;
	
	printf("Initializing port %u... ", port);
	fflush(stdout);

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	// 获取网卡信息
	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

	// 配置port_conf
	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

	// 设置网卡对应的ring数量
	retval = rte_eth_dev_configure(port, RX_RING_NUM, TX_RING_NUM, &port_conf);
	if (retval != 0)
		return retval;

	// 设置每个ring的描述符数量
	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
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

	// 发送队列配置
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

	// 输出网卡MAC信息
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval != 0)
		return retval;

	printf("Port %u --- MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port, RTE_ETHER_ADDR_BYTES(&addr));

	/* 混杂模式 --- 接收所有数据帧(无论MAC地址是否为自身) */
	retval = rte_eth_promiscuous_enable(port);
	if (retval != 0)
		return retval;

	return 0;
}

// 共享队列初始化-单读单写模式 
static void ring_init()
{
	rx_ring = rte_ring_create("rx_ring", RX_RING_SIZE, 
					rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
	
	tx_ring = rte_ring_create("tx_ring", TX_RING_SIZE, 
					rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
}

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

int main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	unsigned int nb_ports;
	unsigned int nb_lcores;
	uint16_t portid;
	unsigned int worker_id;

	uint16_t recv_port = 0;
	uint16_t send_port = 1;

    // eal环境初始化
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	// 工作核心数量
	nb_lcores = rte_lcore_count();
	if (nb_lcores != 4)
		rte_exit(EXIT_FAILURE, "Error: number of lcores must be 3\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("ports number: %u\n", nb_ports);
	if (nb_ports != 2 )
		rte_exit(EXIT_FAILURE, "Error: number of ports must be 2\n");

    // 分配内存池
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // 初始化网卡
	RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", portid);

	// 共享队列初始化
	ring_init();

	printf("\nStart Processing...\n\n");
	
	// 分配工作核心任务
	worker_id = rte_get_next_lcore(-1, 1, 0);
	rte_eal_remote_launch(recv_pkt, &recv_port, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(parse_pkt, NULL, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(send_pkt, &send_port, worker_id);

	FILE *logfile = fopen("log.txt", "w");
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

	// eal环境释放
	rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}