#ifndef __COMMON_H__
#define __COMMON_H__

#include <time.h>

#define NUM_MBUFS 4096
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 128

#define RX_RING_NUM 1
#define TX_RING_NUM 0
#define RX_RING_SIZE 2048
#define TX_RING_SIZE 0

#define LOG_STATISTIC "/var/log/virtio_statistic.log"
#define LOG_PACKET "/var/log/virtio_packet.log"

/* Ethernet frame types */
#define SD_ETHER_TYPE_IPV4 ((uint16_t)0x0008) /**< IPv4 Protocol. */
#define SD_ETHER_TYPE_IPV6 ((uint16_t)0xDD86) /**< IPv6 Protocol. */
#define SD_ETHER_TYPE_ARP  ((uint16_t)0x0608) /**< Arp Protocol. */
#define SD_ETHER_TYPE_RARP ((uint16_t)0x3580) /**< Reverse Arp Protocol. */
#define SD_ETHER_TYPE_VLAN ((uint16_t)0x0081) /**< IEEE 802.1Q VLAN tagging. */
#define SD_ETHER_TYPE_QINQ ((uint16_t)0xA888) /**< IEEE 802.1ad QinQ tagging. */
#define SD_ETHER_TYPE_QINQ1 ((uint16_t)0x0091) /**< Deprecated QinQ VLAN. */
#define SD_ETHER_TYPE_QINQ2 ((uint16_t)0x0092) /**< Deprecated QinQ VLAN. */
#define SD_ETHER_TYPE_QINQ3 ((uint16_t)0x0093) /**< Deprecated QinQ VLAN. */
#define SD_ETHER_TYPE_PPPOE_DISCOVERY ((uint16_t)0x6388) /**< PPPoE Discovery Stage. */
#define SD_ETHER_TYPE_PPPOE_SESSION ((uint16_t)0x6488) /**< PPPoE Session Stage. */
#define SD_ETHER_TYPE_ETAG ((uint16_t)0x3F89) /**< IEEE 802.1BR E-Tag. */
#define SD_ETHER_TYPE_1588 ((uint16_t)0xF788) /**< IEEE 802.1AS 1588 Precise Time Protocol. */
#define SD_ETHER_TYPE_SLOW ((uint16_t)0x0988) /**< Slow protocols (LACP and Marker). */
#define SD_ETHER_TYPE_TEB  ((uint16_t)0x5865) /**< Transparent Ethernet Bridging. */
#define SD_ETHER_TYPE_LLDP ((uint16_t)0xCC88) /**< LLDP Protocol. */
#define SD_ETHER_TYPE_MPLS ((uint16_t)0x4788) /**< MPLS ethertype. */
#define SD_ETHER_TYPE_MPLSM ((uint16_t)0x4888) /**< MPLS multicast ethertype. */
#define SD_ETHER_TYPE_ECPRI ((uint16_t)0xFEAE) /**< eCPRI ethertype (.1Q supported). */

#define MATCH_TYPE(x, y) ((x) == (uint16_t)(y))

#define NS_PER_S 1000000000
#define TIME_BUFFER_SIZE 30

extern struct timespec start_time;
void curr_time(struct timespec *time);

#endif // !__COMMON_H__