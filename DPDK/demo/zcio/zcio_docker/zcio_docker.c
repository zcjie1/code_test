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

#define MAX_PHY_NIC_NUM 32
struct {
	int phy_nic_num; // 物理网卡数量
	uint16_t nic_id[MAX_PHY_NIC_NUM]; // 物理网卡 port_id
}phy_nic;

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

	// 设置网卡对应的ring数量
	retval = rte_eth_dev_configure(port, RX_RING_NUM, TX_RING_NUM, &port_conf);
	if (retval < 0)
		return retval;

	// // 设置每个ring的描述符数量
	// retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	// if (retval < 0)
	// 	return retval;

	// // 设置每个receive ring对应的内存池
	// int port_socket = rte_eth_dev_socket_id(port);
	// rxconf = dev_info.default_rxconf;
	// rxconf.offloads = port_conf.rxmode.offloads;
	// for (int r = 0; r < RX_RING_NUM; r++) {
	// 	retval = rte_eth_rx_queue_setup(port, r, nb_rxd,
	// 			port_socket, &rxconf, mbuf_pool);
	// 	if (retval < 0)
	// 		return retval;
	// }

	// // 配置发送队列
	// txconf = dev_info.default_txconf;
	// txconf.offloads = port_conf.txmode.offloads;
	// for (int r = 0; r < TX_RING_NUM; r++) {
	// 	retval = rte_eth_tx_queue_setup(port, r, nb_txd,
	// 			port_socket, &txconf);
	// 	if (retval < 0)
	// 		return retval;
	// }

	// retval = rte_eth_dev_set_ptypes(port, RTE_PTYPE_UNKNOWN, NULL, 0);
	// if (retval < 0)
	// 	printf("Port %u, Failed to disable Ptype parsing\n", port);

	// 启动网卡
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;
	
	// retval = rte_eth_promiscuous_enable(port);
	// if (retval < 0)
	// 	return retval;

	// 输出网卡信息
	// struct rte_ether_addr addr;
	// retval = rte_eth_macaddr_get(port, &addr);
	// if (retval < 0)
	// 	return retval;

	// printf("Port %u: \n", port);
    // printf("    MAC: %02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 ":"
	// 		   "%02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 "\n",
	// 		RTE_ETHER_ADDR_BYTES(&addr));
    
    rte_eth_dev_get_name_by_port(port, device_name);
    printf("    Device Name: %s\n", device_name);
    printf("    Driver Name: %s\n\n", dev_info.driver_name);

	// 统计普通物理网卡数量
	phy_nic.nic_id[phy_nic.phy_nic_num] = port;
	phy_nic.phy_nic_num++;
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

static int memory_manager(void *arg __rte_unused)
{	
	while(!force_quit) {
		usleep(10000);
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
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

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
		port_init(portid, mbuf_pool);

	printf("\nStart Processing...\n\n");
	
	// 分配工作核心任务
	worker_id = rte_get_next_lcore(-1, 1, 0);
	rte_eal_remote_launch(memory_manager, NULL, worker_id);

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