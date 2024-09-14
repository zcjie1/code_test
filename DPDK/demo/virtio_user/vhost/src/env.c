
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_vhost.h>
#include <rte_eth_vhost.h>

#include "env.h"

extern bool force_quit;
extern bool start_ready;
extern struct status statistics;
extern uint16_t recv_port;
extern uint16_t send_port;

struct rte_ring *fwd_ring;

// 网卡初始化
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool)
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
	if (dev_info.rx_offload_capa & RTE_ETH_RX_OFFLOAD_SCATTER)
		port_conf.rxmode.offloads |= RTE_ETH_RX_OFFLOAD_SCATTER;

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

	// 设置MTU
	// if(rte_eth_dev_set_mtu(port, 9000) < 0)
	// 	printf("Failed to set MTU on port %u\n", port);

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

// 共享队列初始化
static void ring_init(struct rte_ring **fwd_ring, char *name)
{
	*fwd_ring = rte_ring_create(name, RX_RING_SIZE,
					rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
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
static void signal_init(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
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
		start_ready = true;
		clock_gettime(CLOCK_MONOTONIC, &statistics.start_time);
		printf("Virtio Ready for Receiving Packet\n");
	}
	return 0;
}

static void dpdk_init(int argc, char **argv)
{
	int ret;
	unsigned int nb_ports;
    unsigned int nb_lcores;
    uint16_t portid;
	struct rte_mempool *mbuf_pool;

	force_quit = false;
	start_ready = false;

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
}

static void dpdk_exit(void)
{
	int ret;
	uint16_t portid;
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
	rte_ring_free(fwd_ring);

	rte_eal_cleanup();
}

void vhost_init(int argc, char **argv)
{
	// 信号处理函数初始化
	signal_init();

	// 日志文件及统计信息初始化
	log_init();

	// 网卡Port初始化，Ring队列初始化
	dpdk_init(argc, argv);
}

void vhost_exit(void)
{
	log_exit();
	dpdk_exit();
}