/**
docker run \
    -it --rm \
    --privileged \
    -v /tmp/sock0:/var/run/usvhost0 \
    -v /dev/hugepages:/dev/hugepages \
    dpdk-virtio virtio \
        -l 6-7 -m 1024 \
        --no-pci \
        --vdev=virtio_user0,path=/var/run/usvhost0 \
        --file-prefix=Dvirtio \
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

#define NUM_MBUFS 4096
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 32

#define RX_RING_NUM 1
#define TX_RING_NUM 0
#define RX_RING_SIZE 2048
#define TX_RING_SIZE 0

bool force_quit = false;

static struct rte_ether_addr send_mac = {
    .addr_bytes = { 0x11, 0x11, 0x11, 0x22, 0x22, 0x22 }
};

static bool is_same_mac(struct rte_ether_addr *a, struct rte_ether_addr *b)
{
	for(int i = 0; i < RTE_ETHER_ADDR_LEN; i++) {
		if(a->addr_bytes[i] != b->addr_bytes[i])
			return false;
	}

	return true;
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

// 输出数据包的元数据和字符串消息至挂载文件
static int process_pkt(__rte_unused void *arg)
{
	struct rte_mbuf *pkt;
	struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	struct rte_ether_addr src_mac, dst_mac;
	uint16_t ether_type;
	uint32_t src_ip, dst_ip;
    void *data;
	int ret;

    char *msg = (char *)malloc(sizeof(char) * 256);
	msg[255] = '\0';

    uint16_t port = 0;
    struct rte_mbuf *bufs[BURST_SIZE];
	int nb_rx = 0;

	while (!force_quit) {
		nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
		if (unlikely(nb_rx == 0))
			continue;
        
        for(int i = 0; i < nb_rx; i++) {
            pkt = bufs[i];

            // 解析二层网络
            eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
            src_mac = eth_hdr->src_addr;
            dst_mac = eth_hdr->dst_addr;
            ether_type = eth_hdr->ether_type;

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

            rte_pktmbuf_free(pkt);
        }
	}

	free(msg);
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
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval < 0)
		return retval;

	printf("Port %u: \n", port);
    printf("    MAC: %02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 ":"
			   "%02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 "\n",
			RTE_ETHER_ADDR_BYTES(&addr));
    
    char *name = (char *)malloc(sizeof(char) * 512);
    rte_eth_dev_get_name_by_port(port, name);
    printf("    Device Name: %s\n", name);
    printf("    Driver Name: %s\n\n", dev_info.driver_name);

    free(name);

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
	rte_eal_remote_launch(process_pkt, NULL , worker_id);

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