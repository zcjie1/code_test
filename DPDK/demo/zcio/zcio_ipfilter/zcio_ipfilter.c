
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
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_hash.h>
#include <rte_jhash.h>

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 512

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define MAX_BURST_NUM 64

#define ALLOC_NUM(x) (((x) > 32768) ? 0 : ((x) + 32767))
#define FREE_MAGIC_NUM 114514

#define mbuf_eth_hdr(m) rte_pktmbuf_mtod(m, struct rte_ether_hdr*)
#define mbuf_ip_hdr(m) rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr*, sizeof(struct rte_ether_hdr))

bool force_quit = false;

#define MAX_NIC_NUM 16
struct nic_info{
	int nic_num; // 网卡数量
	uint16_t portid[MAX_NIC_NUM]; // 网卡 port_id
};

static struct nic_info zcio_nic;
static char *ip_blacklist_str[15] = {
    "10.10.5.132", "10.10.5.45" , "10.10.5.200", "10.10.5.87" , "10.10.5.234",
    "10.10.5.112", "10.10.5.67" , "10.10.5.150", "10.10.5.99" , "10.10.5.213",
    "10.10.5.104", "10.10.5.56" , "10.10.5.188", "10.10.5.121", "10.10.5.245",
    // "10.10.5.78" , "10.10.5.162", "10.10.5.147", "10.10.5.33" , "10.10.5.108",
};
static uint32_t ip_blacklist_num[15];
static struct rte_hash *ip_blacklist_table;

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

static void raw_packet_free_client(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **tx_pkts, const uint16_t nb_pkts)
{
    if(tx_pkts == NULL)
        return;
    
    for(int i = 0; i < nb_pkts; i++) {
        tx_pkts[i]->dynfield1[0] = FREE_MAGIC_NUM;
    }
    
    int ret = 0;
    int avail_num = nb_pkts;
    int sent_num = 0;
    while(avail_num > 0) {
        ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts + sent_num, avail_num);
        avail_num -= ret;
        sent_num += ret;
    }
}

static void loop_tx(uint16_t port_id, uint16_t queue_id, 
	struct rte_mbuf **tx_pkts, uint16_t nb_pkts)
{
	uint16_t ret = 0;
	uint16_t nb_sent = 0;

	ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts, nb_pkts);
	while(ret < nb_pkts && !force_quit) {
		nb_sent += ret;
		nb_pkts -= ret;
		ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts + nb_sent, nb_pkts);
	}
}

static int ip_filter(void *arg __rte_unused)
{
	if(zcio_nic.nic_num != 1) {
		printf("Error: The number of zcio nic is not 1\n");
		return -1;
	}

    uint16_t portid = zcio_nic.portid[0];
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_rx;
    int ret = 0;
	
	struct rte_ipv4_hdr *ipv4_hdr;
	uint32_t srcip;
	int start_idx = 0;
	int tx_len = 0;

	void *hash_data;

	while(!force_quit) {
		nb_rx = rte_eth_rx_burst(portid, 0, bufs, MAX_BURST_NUM);
		if(nb_rx == 0)
			continue;
		printf("Received %d packets\n", nb_rx);
		for(int i = 0; i < nb_rx; i++) {
			ipv4_hdr = mbuf_ip_hdr(bufs[i]);
			srcip = ipv4_hdr->src_addr;
			ret = rte_hash_lookup_data(ip_blacklist_table, &srcip, &hash_data);
			// printf("key: %u\n", srcip);
			// printf("Hash loop up ret: %d\n", ret);
			if(ret >= 0) {
				// printf("Send %d packets\n", tx_len);
				loop_tx(portid, 0, bufs + start_idx, tx_len);
				printf("Drop a packet from %s\n", inet_ntoa(*(struct in_addr *)&srcip));
				raw_packet_free_client(portid, 0, &bufs[i], 1);
				start_idx = i + 1;
				tx_len = 0;
			}else {
				tx_len += 1;
			}
		}
		// printf("Send %d packets\n", tx_len);
		if(start_idx < nb_rx)
			loop_tx(portid, 0, bufs + start_idx, tx_len);
		start_idx = 0;
		tx_len = 0;
	}

	rte_hash_free(ip_blacklist_table);
	
	return 0;
}

int main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool = NULL;
	unsigned int nb_ports = 0;
	unsigned int nb_lcores = 0;
	uint16_t portid;
	unsigned int worker_id = -1;
	int ret = 0;
	
	force_quit = false;
    
    // eal环境初始化
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	// 工作核心数量
	nb_lcores = rte_lcore_count();
	if (nb_lcores < 1)
		rte_exit(EXIT_FAILURE, "Error: The number of work cores is insufficient\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("ports number: %u\n", nb_ports);
	if (nb_ports < 1)
		rte_exit(EXIT_FAILURE, "Error: The number of ports is insufficient\n");
	
	// 初始化IP地址黑名单哈希表
	struct rte_hash_parameters params = {
        .name = "blackip",
        .entries = 256,
        .reserved = 0,
        .key_len = 1,
        .hash_func = (rte_hash_function)rte_jhash_32b,
        .hash_func_init_val = 1,
        .socket_id = rte_socket_id(),
        .extra_flag = 0,
    };
    ip_blacklist_table = rte_hash_create(&params);
	if(ip_blacklist_table == NULL) {
		printf("Error: Fail to create hash table -- %s\n", strerror(errno));
	}
    for(int i = 0; i < 15; i++) {
        inet_pton(AF_INET, ip_blacklist_str[i], &ip_blacklist_num[i]);
		// printf("Add key %u\n", ip_blacklist_num[i]);
        ret = rte_hash_add_key_data(ip_blacklist_table, &ip_blacklist_num[i], NULL);
		if(ret != 0) {
			printf("Error: Fail to add key %s\n", ip_blacklist_str[i]);
		}
		
		// void *data;
		// uint32_t key = ip_blacklist_num[i];
		// ret = rte_hash_lookup_data(ip_blacklist_table, &key, &data);
		// printf("Lookup ret: %d\n", ret);
    }

	// 分配内存池
	mbuf_pool = rte_pktmbuf_pool_create("share_pool", 1,
		0, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
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
	
	// 分配工作核心任务
	worker_id = rte_get_next_lcore(-1, 1, 0);
	rte_eal_remote_launch(ip_filter, NULL, worker_id);
	
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