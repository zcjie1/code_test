#ifndef __ENV_H__
#define __ENV_H__

#define NUM_MBUFS (8192 * 2)
#define MBUF_CACHE_SIZE 0
#define DEFAULT_PKTMBUF_SIZE (2048 + 128)

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 4096
#define TX_RING_SIZE 4096

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include "iniparser/dictionary.h"
#include "iniparser/iniparser.h"

struct nic_info{
	uint16_t portid;
	char *ipaddr_str;
	uint32_t ipaddr; // big-endian ipaddr
	char *tx_name; // malloc'd string
	struct rte_ring *tx_ring;
};

#define MAX_NIC_NUM 16
struct nic{
	int nic_num;
	struct nic_info info[MAX_NIC_NUM];
};

struct route_entry {
	uint32_t ipaddr; // big-endian destination ipaddr
	struct nic_info *info; // next hop interface
};

struct route_table {
	uint32_t entry_num;
	struct route_entry entry[64];
};

struct config {
	bool force_quit;
	unsigned int curr_worker;

	/* nic info */
	struct nic phy_nic;
	struct nic virtual_nic;
	uint32_t pdev_ipaddr_table[RTE_MAX_ETHPORTS]; // big-endian ipaddr
	uint32_t vdev_ipaddr_table[RTE_MAX_ETHPORTS]; // big-endian ipaddr

	struct rte_mempool *mbuf_pool;
	struct route_table rtable;
};

void nic_txring_init(struct nic_info *nic);
void nic_txring_release(struct nic_info *nic);
void route_table_init(void);
int virtual_host_init(int argc, char **argv);

#endif // !__ENV_H__