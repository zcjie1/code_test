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
#include <rte_fib.h>
#include <rte_hash.h>
#include <rte_malloc.h>
#include "iniparser/dictionary.h"
#include "iniparser/iniparser.h"

struct nic_info{
	uint16_t portid;
	uint32_t ipaddr; // host-endian ipaddr
	uint32_t netmask; // host-endian netmask
	char *tx_name; // malloc'd string
	struct rte_ring *tx_ring;
};

#define MAX_NIC_NUM 16
struct nic{
	int nic_num;
	struct nic_info info[MAX_NIC_NUM];
};

struct config {
	bool force_quit;
	unsigned int curr_worker;

	/* nic info */
	struct nic phy_nic;
	struct nic virtual_nic;
	uint32_t pdev_ipaddr_table[RTE_MAX_ETHPORTS]; // host-endian ipaddr
	uint32_t pdev_netmask_table[RTE_MAX_ETHPORTS]; // host-endian netmask
	uint32_t vdev_ipaddr_table[RTE_MAX_ETHPORTS]; // host-endian ipaddr
	uint32_t vdev_netmask_table[RTE_MAX_ETHPORTS]; // host-endian netmask

	struct rte_mempool *mbuf_pool;
	struct rte_fib *fib;
	struct rte_hash *arp_table;
};

int virtual_host_init(int argc, char **argv);
int virtual_host_destroy(void);

#endif // !__ENV_H__