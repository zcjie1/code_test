#include "env.h"

#define MIN_LCORES_NUM 4
#define MIN_PORTS_NUM 2

struct config global_cfg = {
	.force_quit = false, 
	.curr_worker = -1,
	.mbuf_pool = NULL,
};

static int port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = {0};
	struct rte_eth_dev_info dev_info;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int ret;
	char device_name[256];
	struct nic *virtual_nic = &global_cfg.virtual_nic;
	struct nic *phy_nic = &global_cfg.phy_nic;
	struct rte_eth_rxconf rxconf;
	struct rte_eth_txconf txconf;
	
	printf("Initializing port %u...\n", port);

	if (!rte_eth_dev_is_valid_port(port)) {
		printf("%s: Invalid port %u\n", __func__, port);
		return -1;
	}

	ret = rte_eth_dev_info_get(port, &dev_info);
	if (ret != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-ret));
		return ret;
	}

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
	if (dev_info.rx_offload_capa & RTE_ETH_RX_OFFLOAD_SCATTER)
		port_conf.rxmode.offloads |= RTE_ETH_RX_OFFLOAD_SCATTER;

	ret = rte_eth_dev_configure(port, RX_RING_NUM, TX_RING_NUM, &port_conf);
	if (ret < 0) {
		printf("Cannot configure device: err=%d, port=%u\n",
				ret, port);
		return ret;
	}
	
	ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (ret < 0) {
		printf("Cannot adjust number of descriptors: err=%d, port=%u\n",
				ret, port);
		return ret;
	}
		
	int port_socket = rte_eth_dev_socket_id(port);
	rxconf = dev_info.default_rxconf;
	rxconf.offloads = port_conf.rxmode.offloads;
	for (int r = 0; r < RX_RING_NUM; r++) {
		ret = rte_eth_rx_queue_setup(port, r, nb_rxd,
				port_socket, &rxconf, mbuf_pool);
		if (ret < 0) {
			printf("Cannot setup receive queue %u for port %u\n", r, port);
			return ret;
		}	
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	for (int r = 0; r < TX_RING_NUM; r++) {
		ret = rte_eth_tx_queue_setup(port, r, nb_txd,
				port_socket, &txconf);
		if (ret < 0) {
			printf("Cannot setup transmit queue %u for port %u\n", r, port);
			return ret;
		}	
	}

	ret = rte_eth_dev_start(port);
	if (ret < 0) {
		printf("Cannot start device: err=%d, port=%u\n", ret, port);
		return ret;
	}
		
	ret = rte_eth_promiscuous_enable(port);
	if (ret < 0) {
		printf("Cannot enable promiscuous mode: err=%d, port=%u\n", ret, port);
		return ret;
	}
		
	// 输出网卡信息
	struct rte_ether_addr addr;
	ret = rte_eth_macaddr_get(port, &addr);
	if (ret < 0) {
		printf("Error reading MAC address for port %d\n", port);
		return ret;
	}

	printf("Port %u: \n", port);
    printf("    MAC: %02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 ":"
			   "%02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 "\n",
			RTE_ETHER_ADDR_BYTES(&addr));
    
    rte_eth_dev_get_name_by_port(port, device_name);
    printf("    Device Name: %s\n", device_name);
    printf("    Driver Name: %s\n\n", dev_info.driver_name);

	// 统计网卡数量, 分配IP地址
	// if(strcmp(dev_info.driver_name, "net_zcio") == 0 || 
	// 	strcmp(dev_info.driver_name, "net_vhost") == 0) {
	// 	virtual_nic->info[virtual_nic->nic_num].portid = port;
	// 	inet_pton(AF_INET, virtual_ip_list[virtual_nic->nic_num], &virtual_nic->info[virtual_nic->nic_num].ipaddr);
	// 	virtual_nic->info[virtual_nic->nic_num].ipaddr_str = virtual_ip_list[virtual_nic->nic_num];
	// 	nic_txring_init(&virtual_nic->info[virtual_nic->nic_num]);
	// 	virtual_nic->nic_num++;
	// }else {
	// 	phy_nic->info[phy_nic->nic_num].portid = port;
	// 	inet_pton(AF_INET, phy_ip_list[phy_nic->nic_num], &phy_nic->info[phy_nic->nic_num].ipaddr);
	// 	phy_nic->info[phy_nic->nic_num].ipaddr_str = phy_ip_list[phy_nic->nic_num];
	// 	nic_txring_init(&phy_nic->info[phy_nic->nic_num]);
	// 	phy_nic->nic_num++;
	// }
	
	return 0;
}

void nic_txring_init(struct nic_info *nic)
{
	nic->tx_name = (char *)malloc(64);
	sprintf(nic->tx_name, "tx_ring_%d", nic->portid);
	if(nic->portid == 0) {
		nic->tx_ring = rte_ring_create(nic->tx_name, TX_RING_SIZE, 
			rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
	}else {
		nic->tx_ring = rte_ring_create(nic->tx_name, TX_RING_SIZE, 
			rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
	}
}

void nic_txring_release(struct nic_info *nic)
{
	rte_ring_free(nic->tx_ring);
	free(nic->tx_name);
}

void route_table_init(void)
{
	struct route_entry *entry;
	uint32_t ipaddr;
	uint16_t portid;
	
	struct route_table *rtable = &global_cfg.rtable;
	struct nic *virtual_nic = &global_cfg.virtual_nic;
	struct nic *phy_nic = &global_cfg.phy_nic;

	/* 初始化路由表 */
	entry = &rtable->entry[rtable->entry_num];
	
	// 物理网卡路由
	for(int i = 0; i < phy_nic->nic_num; i++) {
		entry = &rtable->entry[rtable->entry_num];
		entry->ipaddr = phy_nic->info[i].ipaddr;
		entry->info = &phy_nic->info[i];
		rtable->entry_num++;
	}
	
	// virtual 网卡路由
	for(int i = 0; i < virtual_nic->nic_num; i++) {
		entry = &rtable->entry[rtable->entry_num];
		entry->ipaddr = virtual_nic->info[i].ipaddr;
		entry->info = &virtual_nic->info[i];
		rtable->entry_num++;
	}
	// printf("route table entry_num: %u\n", rtable->entry_num);
}

static void parse_inifile(int _argc, char **_argv)
{
	int argc = 1;
	char *argv[32] = {_argv[0]};

	char lcore_list[64];
	char memory[64];
	char proc_type[64];
	char no_pci[] = "--no-pci";
	char no_shvdev[] = "--no-shvdev";
	uint16_t nb_vdevs = 0;
	uint16_t vdev_list[RTE_MAX_ETHPORTS];
	char vdev_param[RTE_MAX_ETHPORTS][128];
	dictionary *iniparam = iniparser_load(_argv[1]);

	const char *strvalue;
	char *tmpvalue;
	int intvalue;

	strvalue = iniparser_getstring(iniparam, "dpdk:lcore_list", NULL);
	if(strvalue == NULL)
		rte_exit(EXIT_FAILURE, "Error: No lcore_list\n");
	sprintf(lcore_list, "-l %s", strvalue);
	argv[argc++] = lcore_list;

	strvalue = iniparser_getstring(iniparam, "dpdk:memory", NULL);
	if(strvalue == NULL)
		rte_exit(EXIT_FAILURE, "Error: No memory\n");
	sprintf(memory, "-m %s", strvalue);
	argv[argc++] = memory;

	strvalue = iniparser_getstring(iniparam, "dpdk:proc_type", NULL);
	if(strvalue == NULL)
		rte_exit(EXIT_FAILURE, "Error: No proc_type\n");
	sprintf(proc_type, "--proc-type=%s", strvalue);
	argv[argc++] = proc_type;

	intvalue = iniparser_getint(iniparam, "dpdk:no_pci", 0);
	if(intvalue == 1)
		argv[argc++] = no_pci;
	
	intvalue = iniparser_getint(iniparam, "dpdk:no_shvdev", 0);
	if(intvalue == 1)
		argv[argc++] = no_shvdev;

	/* vdev_list */
	char *token;
	strvalue = iniparser_getstring(iniparam, "dpdk:vdev_list", NULL);
	if(strvalue == NULL)
		rte_exit(EXIT_FAILURE, "Error: No vdev_list\n");
	tmpvalue = strdup(strvalue);
	token = strtok(tmpvalue, ",");
	while(token != NULL) {
		if(nb_vdevs >= RTE_MAX_ETHPORTS)
			rte_exit(EXIT_FAILURE, "Error: vdev_list is too long\n");
		vdev_list[nb_vdevs++] = (uint16_t)atoi(token);
		token = strtok(NULL, ",");
	}
	free(tmpvalue);

	/* pdev_ipaddr */
	int count = 0;
	strvalue = iniparser_getstring(iniparam, "dpdk:pdev_ipaddr", NULL);
	if(strvalue == NULL)
		rte_exit(EXIT_FAILURE, "Error: No pdev_ipaddr\n");
	tmpvalue = strdup(strvalue);
	token = strtok(tmpvalue, ",");
	while(token != NULL) {
		global_cfg.pdev_ipaddr_table[count++] = (uint32_t)inet_addr(token);
		token = strtok(NULL, ",");
	}
	free(tmpvalue);

	/* vdevX */
	char vdev_key[64];
	for(int i = 0; i < nb_vdevs; i++) {
		sprintf(vdev_key, "vdev%d:vdev_param", i);
		strvalue = iniparser_getstring(iniparam, vdev_key, NULL);
		if(strvalue == NULL)
			rte_exit(EXIT_FAILURE, "Error: No vdev_param in vdev%d\n", i);
		sprintf(vdev_param[i], "--vdev=%s", strvalue);
		argv[argc++] = vdev_param[i];

		sprintf(vdev_key, "vdev%d:ipaddr", i);
		strvalue = iniparser_getstring(iniparam, vdev_key, NULL);
		global_cfg.vdev_ipaddr_table[i] = (uint32_t)inet_addr(strvalue);
	}

	for(int i = 0; i < argc; i++)
		printf("%s\n", argv[i]);
	
	rte_exit(EXIT_SUCCESS, "Success: parse ini file\n");
}

int virtual_host_init(int argc, char **argv)
{	
	if(argc < 2)
		rte_exit(EXIT_FAILURE, "Error: No config file\n");
	
	parse_inifile(argc, argv);

	int ret = 0;
	uint16_t portid;
	unsigned int nb_lcores;
	unsigned int nb_ports;
	unsigned int worker_id;
	struct rte_mempool *mbuf_pool;
	
	// 工作核心数量
	nb_lcores = rte_lcore_count();
	printf("Lcores Number: %u\n", nb_lcores);
	if (nb_lcores < MIN_LCORES_NUM)
		rte_exit(EXIT_FAILURE, "Error: Number of work cores is insufficient\n");
	
	// 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("Ports Number: %u\n", nb_ports);
	if (nb_ports < MIN_PORTS_NUM)
		rte_exit(EXIT_FAILURE, "Error: Number of ports is insufficient\n");
	
	// 分配内存池, mbuf_pool_0 兼容 f-stack
	mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool_0", 16*NUM_MBUFS*nb_ports, 
		MBUF_CACHE_SIZE, 0, DEFAULT_PKTMBUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	global_cfg.mbuf_pool = mbuf_pool;
	
	// 初始化网卡
	RTE_ETH_FOREACH_DEV(portid) {
		ret = port_init(portid, mbuf_pool);
		if(ret != 0) {
			global_cfg.force_quit = true;
			printf("\nError: Fail to init port %"PRIu16"\n", portid);
			goto out;
		}
	}
	
	route_table_init();

	return 0;

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

	// 释放网卡发送队列
	for(int i = 0; i < global_cfg.virtual_nic.nic_num; i++)
		nic_txring_release(&global_cfg.virtual_nic.info[i]);
	for(int i = 0; i < global_cfg.phy_nic.nic_num; i++)
		nic_txring_release(&global_cfg.phy_nic.info[i]);
	
	// 释放内存池
	rte_mempool_free(mbuf_pool);

	if(ret != 0)
		return -1;
	return 0;
}