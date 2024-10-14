#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "env.h"
#include <rte_dev.h>
#include <rte_ether.h>

#define ARP_REQUEST    1
#define ARP_REPLY      2

struct arphdr {
    uint16_t  ar_hrd;
    uint16_t  ar_pro;
    uint8_t   ar_hln;
    uint8_t   ar_pln;
    uint16_t  ar_op;
    struct rte_ether_addr ar_sha;
    uint32_t  ar_sip;
    struct rte_ether_addr ar_tha;
    uint32_t  ar_tip;
}__rte_packed;

#define mbuf_eth_hdr(m) rte_pktmbuf_mtod(m, struct rte_ether_hdr*)
#define mbuf_arphdr(m) rte_pktmbuf_mtod_offset(m, struct arphdr*, sizeof(struct rte_ether_hdr))
#define mbuf_ip_hdr(m) rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr*, sizeof(struct rte_ether_hdr))
#define mbuf_udp_hdr(m) \
    rte_pktmbuf_mtod_offset(m, struct rte_udp_hdr*, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr))

#define MAX_BURST_NUM 32

// int route_process(void *arg __rte_unused);
int phy_nic_receive(void *arg __rte_unused);
int phy_nic_send(void *arg __rte_unused);
int zcio_nic_receive(void *arg);
int zcio_nic_send(void *arg);

#endif // !__PROCESS_H__