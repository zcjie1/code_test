#include "env.h"

struct nic phy_nic;
struct nic zcio_nic;
struct route_table rtable;

// zcio网卡IP地址分配
char *zcio_ip_list[] = {
	"192.168.2.1", "192.168.2.2",
	"192.168.2.3", "192.168.2.4",
};

// 物理网卡IP地址分配
char *phy_ip_list[] = {
	"10.10.4.119",
};
// 网卡初始化
int port_init(uint16_t port, struct rte_mempool *mbuf_pool)
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
		zcio_nic.info[zcio_nic.nic_num].portid = port;
		inet_pton(AF_INET, zcio_ip_list[zcio_nic.nic_num], &zcio_nic.info[zcio_nic.nic_num].ipaddr);
		zcio_nic.info[zcio_nic.nic_num].ipaddr_str = zcio_ip_list[zcio_nic.nic_num];
		nic_txring_init(&zcio_nic.info[zcio_nic.nic_num]);
		zcio_nic.nic_num++;
	}else {
		phy_nic.info[phy_nic.nic_num].portid = port;
		inet_pton(AF_INET, phy_ip_list[phy_nic.nic_num], &phy_nic.info[phy_nic.nic_num].ipaddr);
		phy_nic.info[phy_nic.nic_num].ipaddr_str = phy_ip_list[phy_nic.nic_num];
		nic_txring_init(&phy_nic.info[phy_nic.nic_num]);
		phy_nic.nic_num++;
	}
	
	return 0;
}

void nic_txring_init(struct nic_info *nic)
{
	nic->tx_name = (char *)malloc(64);
	sprintf(nic->tx_name, "tx_ring_%d", nic->portid);
	nic->tx_ring = rte_ring_create(nic->tx_name, TX_RING_SIZE, rte_socket_id(),
			RING_F_SP_ENQ| RING_F_SC_DEQ);
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

	/* 初始化路由表 */

	for(int i = 0; i < zcio_nic.nic_num; i++) {
		entry = &rtable.entry[rtable.entry_num];
		entry->ipaddr = zcio_nic.info[i].ipaddr;
		entry->portid = zcio_nic.info[i].portid;
		rtable.entry_num++;
	}
	
	// 若目标IP地址为0.0.0.0，转发至物理网卡，发出
	entry = &rtable.entry[rtable.entry_num];
	entry->ipaddr = phy_nic.info[0].ipaddr;
	entry->portid = phy_nic.info[0].portid;
	rtable.entry_num++;
}