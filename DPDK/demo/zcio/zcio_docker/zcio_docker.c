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
#include <asm-generic/fcntl.h>

#define SOCKET_PATH "/tmp/zcio_unix.socket"

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 512

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

bool force_quit = false;
static uint64_t pkt_num = 0;

#define MAX_NIC_NUM 16
struct nic_info{
	int nic_num; // 网卡数量
	uint16_t portid[MAX_NIC_NUM]; // 网卡 port_id
};

static struct nic_info zcio_nic;

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

	if(strcmp(dev_info.driver_name, "net_zcio") == 0) {
		zcio_nic.portid[zcio_nic.nic_num] = port;
		zcio_nic.nic_num++;
	}
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

static void parse_pkt(struct rte_mbuf *pkt)
{
	struct rte_ether_addr src_mac, dst_mac;
	uint16_t ether_type;
	
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
	src_mac = eth_hdr->src_addr;
	dst_mac = eth_hdr->dst_addr;
	ether_type = eth_hdr->ether_type;
	uint64_t *data_ptr = (uint64_t *)(eth_hdr + 1);
	uint64_t raw_data = *data_ptr;
	uint64_t data = rte_be_to_cpu_64(raw_data);
	printf("--------------------------PACKET %lu--------------------------\n", pkt_num++);
	printf("src mac %02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 "\n",
		RTE_ETHER_ADDR_BYTES(&src_mac));
	printf("dst mac %02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 "\n",
		RTE_ETHER_ADDR_BYTES(&dst_mac));
	printf("ether type %04"PRIx16 "\n", ether_type);
	printf("raw data %016"PRIx64 "\n", raw_data);
	printf("data %016"PRIx64 "\n", data);
	printf("--------------------------------------------------------------\n");
	*data_ptr = rte_cpu_to_be_64(data+1);
}

static int client_test(void *arg __rte_unused)
{	
	if(zcio_nic.nic_num != 1) {
		printf("Error: The number of zcio nic is not 1\n");
		return -1;
	}
	
	int ret = 0;
	struct rte_mbuf *recv;
	
	while(!force_quit) {
		ret = rte_eth_rx_burst(zcio_nic.portid[0], 0, &recv, 1);
		if(ret) {
			parse_pkt(recv);
			ret = rte_eth_tx_burst(zcio_nic.portid[0], 0, &recv, 1);
			while(!ret && !force_quit)
				ret = rte_eth_tx_burst(zcio_nic.portid[0], 0, &recv, 1);
			continue;
		}
		usleep(1);
	}
	return 0;
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
	if (nb_lcores < 1)
		rte_exit(EXIT_FAILURE, "Error: The number of work cores is insufficient\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("ports number: %u\n", nb_ports);
	if (nb_ports < 1 )
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
			printf("\nCannot init port %"PRIu16"\n", portid);
			goto out;
		}

	printf("\nStart Processing...\n\n");
	
	// 分配工作核心任务
	worker_id = rte_get_next_lcore(-1, 1, 0);
	rte_eal_remote_launch(client_test, NULL, worker_id);
	
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