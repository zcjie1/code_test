#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "env.h"
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

void arp_process(struct rte_mbuf *m, struct nic_info *if_input);

#endif // !__PROTOCOL_H__