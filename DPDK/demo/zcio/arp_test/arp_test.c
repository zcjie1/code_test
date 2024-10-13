/**
 * 程序功能概述：
 * - 初始化所有网卡（包括vhost网卡和物理网卡）
 * - 分配一个巨大的内存池
 * - 分配一个内存池管理模块，负责与服务容器通信，交换大页数据
 * - ...
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/fcntl.h>

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

#define MEMCTL_PATH "/tmp/memctl.sock"

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 512

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

bool force_quit = false;

#define MAX_NIC_NUM 16
struct nic_info{
	int nic_num; // 网卡数量
	uint16_t portid[MAX_NIC_NUM]; // 网卡 port_id
};

#define ETH_ADDR_LEN        6
#define ETH_ADDR_STR_LEN    17

struct eth_addr {
    uint8_t bytes[ETH_ADDR_LEN];
} __attribute__((__packed__));

struct eth_hdr {
    struct eth_addr d_addr;
    struct eth_addr s_addr;
    uint16_t type;
} __attribute__((__packed__));

struct arphdr {
    uint16_t  ar_hrd;
    uint16_t  ar_pro;
    uint8_t   ar_hln;
    uint8_t   ar_pln;

    uint16_t  ar_op;

    struct eth_addr ar_sha;
    uint32_t  ar_sip;
    struct eth_addr ar_tha;
    uint32_t  ar_tip;
} __attribute__((__packed__));

#define mbuf_eth_hdr(m) rte_pktmbuf_mtod(m, struct eth_hdr *)
#define mbuf_arphdr(m) rte_pktmbuf_mtod_offset(m, struct arphdr*, sizeof(struct eth_hdr))
#define mbuf_ip_hdr(m) rte_pktmbuf_mtod_offset(m, struct iphdr*, sizeof(struct eth_hdr))

static struct nic_info phy_nic;
static struct nic_info zcio_nic;

static inline void eth_addr_copy(struct eth_addr *dst, const struct eth_addr *src)
{
    memcpy((void*)dst, (const void*)src, sizeof(struct eth_addr));
}

static inline void eth_hdr_set(struct eth_hdr *eth, uint16_t type, const struct eth_addr *d_addr,
    const struct eth_addr *s_addr)
{
    eth->type = htons(type);
    eth_addr_copy(&eth->d_addr, d_addr);
    eth_addr_copy(&eth->s_addr, s_addr);
}

static inline void arp_set_arphdr(struct arphdr *arp, uint16_t op, uint32_t sip, uint32_t dip,
    const struct eth_addr *sa, const struct eth_addr *da)
{
    arp->ar_hrd = htons(0x01);
    arp->ar_pro = htons(0x0800);
    arp->ar_hln = 0x06;
    arp->ar_pln = 0x04;
    arp->ar_op = htons(op);
    arp->ar_sip = sip;
    arp->ar_tip = dip;
    eth_addr_copy(&arp->ar_sha, sa);
    eth_addr_copy(&arp->ar_tha, da);
}

// 网卡初始化
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf;
	struct rte_eth_dev_info dev_info;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	char device_name[256];

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
	if (dev_info.rx_offload_capa & RTE_ETH_RX_OFFLOAD_SCATTER)
		port_conf.rxmode.offloads |= RTE_ETH_RX_OFFLOAD_SCATTER;

	// 初始化网卡驱动
	retval = rte_eth_dev_configure(port, RX_RING_NUM, TX_RING_NUM, &port_conf);
	if (retval < 0) {
		printf("Cannot configure device: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		
	// 设置每个ring的描述符数量
	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval < 0) {
		printf("Cannot adjust number of descriptors: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		

	// 设置每个receive ring对应的内存池
	int port_socket = rte_eth_dev_socket_id(port);
	rxconf = dev_info.default_rxconf;
	rxconf.offloads = port_conf.rxmode.offloads;
	for (int r = 0; r < RX_RING_NUM; r++) {
		retval = rte_eth_rx_queue_setup(port, r, nb_rxd,
				port_socket, &rxconf, mbuf_pool);
		if (retval < 0) {
			printf("Cannot setup receive queue %u for port %u\n",
					r, port);
			return retval;
		}
			
	}

	// 配置发送队列
	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	for (int r = 0; r < TX_RING_NUM; r++) {
		retval = rte_eth_tx_queue_setup(port, r, nb_txd,
				port_socket, &txconf);
		if (retval < 0) {
			printf("Cannot setup transmit queue %u for port %u\n",
					r, port);
			return retval;
		}
			
	}

	retval = rte_eth_dev_set_ptypes(port, RTE_PTYPE_UNKNOWN, NULL, 0);
	if (retval < 0)
		printf("Port %u, Failed to disable Ptype parsing\n", port);

	// 启动网卡
	retval = rte_eth_dev_start(port);
	if (retval < 0) {
		printf("Cannot start device: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		
	retval = rte_eth_promiscuous_enable(port);
	if (retval < 0) {
		printf("Cannot enable promiscuous mode: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		

	// 输出网卡信息
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval < 0) {
		printf("Error reading MAC address for port %d\n", port);
		return retval;
	}

	printf("Port %u: \n", port);
    printf("    MAC: %02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 ":"
			   "%02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 "\n",
			RTE_ETHER_ADDR_BYTES(&addr));
    
    rte_eth_dev_get_name_by_port(port, device_name);
    printf("    Device Name: %s\n", device_name);
    printf("    Driver Name: %s\n\n", dev_info.driver_name);

	// 统计网卡数量
	if(strcmp(dev_info.driver_name, "net_zcio") == 0) {
		zcio_nic.portid[zcio_nic.nic_num] = port;
		zcio_nic.nic_num++;
	}else {
		phy_nic.portid[phy_nic.nic_num] = port;
		phy_nic.nic_num++;
	}
	
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

static void arp_request_process(struct rte_mbuf *m)
{
	struct in_addr ip_addr;
	inet_pton(AF_INET, "10.10.4.119", &ip_addr);
	const struct eth_addr smac = { 
		.bytes = { 0x00, 0x50, 0x56, 0xbf, 0xa9, 0x7c } 
	};
    struct arphdr *arph = mbuf_arphdr(m);
    if(arph->ar_op == htons((uint16_t)1)) {
        if(arph->ar_tip == (uint32_t)ip_addr.s_addr) {
			struct eth_hdr *eth = mbuf_eth_hdr(m);
			struct arphdr *arph = mbuf_arphdr(m);
			uint32_t sip = arph->ar_tip;
			uint32_t dip = arph->ar_sip;
			struct eth_addr dmac;
			eth_addr_copy(&dmac, &eth->s_addr);
    		eth_hdr_set(eth, (uint16_t)0x0806, &dmac, &smac);
			arp_set_arphdr(arph, 2, sip, dip, &smac, &dmac);
			rte_eth_tx_burst(phy_nic.portid[0], 0, &m, 1);
		}else {
			rte_pktmbuf_free(m);
		}
    }else {
		rte_pktmbuf_free(m);
	}
}

static int arp_test(void *arg)
{
    if(phy_nic.nic_num != 1)
        return -1;
    
    struct rte_mempool *mbuf_pool = arg;
    struct rte_mbuf *m;
	int nb_rx = 0;
    
    struct eth_hdr *eth = NULL;
    struct iphdr *iph = NULL;
    while(!force_quit) {
        nb_rx = rte_eth_rx_burst(phy_nic.portid[0], 0, &m, 1);
		if (unlikely(nb_rx == 0))
			continue;
        eth = mbuf_eth_hdr(m);
        iph = mbuf_ip_hdr(m);
        if(eth->type == htons((uint16_t)0x0806)) {
			printf("receive arp request\n");
            arp_request_process(m);
        }else {
			rte_pktmbuf_free(m);
		}
        
    }
}

int main(int argc, char *argv[])
{
    struct rte_mempool *mbuf_pool;
	unsigned int nb_ports;
	unsigned int nb_lcores;
	uint16_t portid;
	unsigned int worker_id;

	force_quit = false;

    // eal环境初始化
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	// 工作核心数量
	nb_lcores = rte_lcore_count();
	if (nb_lcores < 3)
		rte_exit(EXIT_FAILURE, "Error: The number of work cores is insufficient\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("ports number: %u\n", nb_ports);
	if (nb_ports < 1)
		rte_exit(EXIT_FAILURE, "Error: The number of ports is insufficient\n");

    // 分配内存池
	mbuf_pool = rte_pktmbuf_pool_create("share_pool", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // 初始化网卡
	RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid, mbuf_pool) != 0) {
			force_quit = true;
			printf("\nError: Fail to init port %"PRIu16"\n", portid);
			goto out;
		}
		
	printf("\nStart Processing...\n\n");

	// if(zcio_nic.nic_num < 2)
	// 	rte_exit(EXIT_FAILURE, "Error: The number of zcio nic is not enough\n");

	// 分配工作核心任务
	worker_id = rte_get_next_lcore(-1, 1, 0);
	rte_eal_remote_launch(arp_test, mbuf_pool, worker_id);
	
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

 out:
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

	// eal环境释放
	rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}