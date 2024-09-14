#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdbool.h>
#include <rte_ether.h>
#include <rte_mbuf.h>

#define SD_ETHER_TYPE_IPV4 ((uint16_t)0x0008)  /**< IPv4 Protocol. */

// 二层网络类型
struct packet_type {
	rte_be16_t  type;	/* This is really htons(ether_type). */
	int (*func) (struct rte_mbuf *, struct packet_type *);
};

// 三层网络类型
struct net_protocol {
    uint8_t type;
    int	(*handler)(struct rte_mbuf *);
};

#endif // !__PROTOCOL_H__