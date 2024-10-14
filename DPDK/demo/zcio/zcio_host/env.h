#ifndef __ENV_H__
#define __ENV_H__

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 512

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>

struct nic_info{
	uint16_t portid;
	char *ipaddr_str;
	uint32_t ipaddr; // 大端序IP地址
	char *tx_name; // malloc'd
	struct rte_ring *tx_ring;
};

#define MAX_NIC_NUM 16
struct nic{
	int nic_num; // 网卡数量
	struct nic_info info[MAX_NIC_NUM];
};

struct route_entry {
	uint32_t ipaddr; // 大端序IP地址, 数据包的原dst地址
	struct nic_info *info; // 下一跳的端口信息
};

struct route_table {
	uint32_t entry_num;
	struct route_entry entry[64];
};

int port_init(uint16_t port, struct rte_mempool *mbuf_pool);
void nic_txring_init(struct nic_info *nic);
void nic_txring_release(struct nic_info *nic);
void route_table_init(void);

#endif // !__ENV_H__