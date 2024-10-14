#include "process.h"
#include <byteswap.h>

extern bool force_quit;
extern struct nic phy_nic;
extern struct nic zcio_nic;
extern struct route_table rtable;
extern struct rte_ring *route_ring;

struct nic_info* find_next_port(struct rte_mbuf *m)
{
    struct route_table *table = &rtable;
    struct rte_ipv4_hdr *ipv4_hdr = mbuf_ip_hdr(m);
    uint32_t dstip = ipv4_hdr->dst_addr;

    for(int i = 0; i < table->entry_num; i++) {
        if(dstip == table->entry[i].ipaddr) {
            ipv4_hdr->dst_addr = table->entry[i].info->ipaddr;
            return table->entry[i].info;
        }
    }

    return NULL; 
}

static inline void eth_hdr_set(struct rte_ether_hdr *eth, uint16_t type, 
        struct rte_ether_addr *src_mac, struct rte_ether_addr *dst_mac)
{
    eth->ether_type = htons(type);
    rte_ether_addr_copy(src_mac, &eth->src_addr);
    rte_ether_addr_copy(dst_mac, &eth->dst_addr);
}

static inline void arp_set_arphdr(struct arphdr *arp, uint16_t op, uint32_t sip, uint32_t dip,
    struct rte_ether_addr *sa, struct rte_ether_addr *da)
{
    arp->ar_hrd = htons(0x01);
    arp->ar_pro = htons(0x0800);
    arp->ar_hln = 0x06;
    arp->ar_pln = 0x04;
    arp->ar_op = htons(op);
    arp->ar_sip = sip;
    arp->ar_tip = dip;
    rte_ether_addr_copy(sa, &arp->ar_sha);
    rte_ether_addr_copy(da, &arp->ar_tha);
}

static void arp_process(struct rte_mbuf *m)
{
	struct in_addr ip_addr;
    ip_addr.s_addr = (in_addr_t)phy_nic.info[0].ipaddr;
    uint32_t sip;
    uint32_t dip;
    struct rte_ether_addr src_mac;
    struct rte_ether_addr dst_mac;
    struct rte_ether_hdr *eth;
    
    uint16_t ret = 0;
    
    rte_eth_macaddr_get(phy_nic.info[0].portid, &src_mac);
    
    struct arphdr *arph = mbuf_arphdr(m);
    if(arph->ar_op == htons(ARP_REQUEST) && 
        arph->ar_tip == (uint32_t)ip_addr.s_addr) {
        eth = mbuf_eth_hdr(m);
        sip = arph->ar_tip;
        dip = arph->ar_sip;
        rte_ether_addr_copy(&eth->src_addr, &dst_mac);
        eth_hdr_set(eth, RTE_ETHER_TYPE_ARP, &src_mac, &dst_mac);
        arp_set_arphdr(arph, ARP_REPLY, sip, dip, &src_mac, &dst_mac);
        ret = rte_eth_tx_burst(phy_nic.info[0].portid, 0, &m, 1);
        if(ret == 0)
            rte_pktmbuf_free(m);
    }else {
		rte_pktmbuf_free(m);
	}
}

int phy_nic_receive(void *arg __rte_unused)
{
    uint16_t portid = phy_nic.info[0].portid;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_rx;
    int ret = 0;

    struct rte_ether_hdr *eth_hdr = NULL;
    struct rte_ipv4_hdr *ipv4_hdr = NULL;
    
    while (!force_quit) {
        nb_rx = rte_eth_rx_burst(portid, 0, bufs, MAX_BURST_NUM);
        if (nb_rx == 0)
            continue;
        for(int i = 0; i < nb_rx; i++) {
            eth_hdr = mbuf_eth_hdr(bufs[i]);
            if(eth_hdr->ether_type == htons(RTE_ETHER_TYPE_ARP)) {
                arp_process(bufs[i]);
            }else if(eth_hdr->ether_type == htons(RTE_ETHER_TYPE_IPV4)) {
                ipv4_hdr = mbuf_ip_hdr(bufs[i]);
                if(ipv4_hdr->hdr_checksum != rte_ipv4_cksum(ipv4_hdr)) {
                    rte_pktmbuf_free(bufs[i]);
                    continue;
                }
                struct nic_info *nic = find_next_port(bufs[i]);
                if(nic == NULL)
                    rte_pktmbuf_free(bufs[i]);
                ret = rte_ring_enqueue(nic->tx_ring, bufs[i]);
                if(ret < 0) {
                    rte_pktmbuf_free(bufs[i]);
                }
            }else {
                rte_pktmbuf_free(bufs[i]);
            }
        }
    }
    return 0;
}

static void phynic_out_process(struct rte_mbuf *m)
{
    struct rte_ether_hdr *eth_hdr = mbuf_eth_hdr(m);
    struct rte_ipv4_hdr *ipv4_hdr = mbuf_ip_hdr(m);
    struct rte_udp_hdr *udp_hdr = mbuf_udp_hdr(m);
    
    // UDP 端口互换
    rte_be16_t tmp_port = udp_hdr->src_port;
    udp_hdr->src_port = udp_hdr->dst_port;
    udp_hdr->dst_port = tmp_port;
    
    // IP 设置
    ipv4_hdr->dst_addr = ipv4_hdr->src_addr;
    ipv4_hdr->src_addr = phy_nic.info[0].ipaddr;
    ipv4_hdr->time_to_live = 64;
    
    // UDP 校验和
    udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ipv4_hdr, (void *)udp_hdr);
    
    // IP 校验和
    ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
    
    // MAC 地址交换
    struct rte_ether_addr tmp_mac;
    rte_ether_addr_copy(&eth_hdr->dst_addr, &tmp_mac);
    rte_ether_addr_copy(&eth_hdr->src_addr, &eth_hdr->dst_addr);
    rte_ether_addr_copy(&tmp_mac, &eth_hdr->src_addr);
}

int phy_nic_send(void *arg __rte_unused)
{
    uint16_t portid = phy_nic.info[0].portid;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    struct rte_ring *tx_ring = phy_nic.info[0].tx_ring;
    uint16_t nb_tx = 0;
    uint16_t ret = 0;

    while(!force_quit) {
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
        for(int i = 0; i < nb_tx; i++) {
            phynic_out_process(bufs[i]);
        }
        ret = rte_eth_tx_burst(portid, 0, bufs, nb_tx);
        while(ret < nb_tx) {
            nb_tx -= ret;
            ret = rte_eth_tx_burst(portid, 0, (bufs + ret), nb_tx);
        }
    }

    nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    while(nb_tx != 0) {
        rte_pktmbuf_free_bulk(bufs, nb_tx);
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    }
    
    return 0;
}

int zcio_nic_receive(void *arg)
{
    struct nic_info *nic = arg;
    uint16_t portid = nic->portid;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_rx;
    int ret = 0;
    
    while (!force_quit) {
        nb_rx = rte_eth_rx_burst(portid, 0, bufs, MAX_BURST_NUM);
        if (nb_rx == 0)
            continue;
        for(int i = 0; i < nb_rx; i++) {
            struct nic_info *info = find_next_port(bufs[i]);
            if(info == NULL)
                rte_pktmbuf_free(bufs[i]);
            ret = rte_ring_enqueue(info->tx_ring, bufs[i]);
            if(ret < 0) {
                rte_pktmbuf_free(bufs[i]);
            }
        }
    }
}

int zcio_nic_send(void *arg)
{
    struct nic_info *nic = arg;
    uint16_t portid = nic->portid;
    struct rte_ring *tx_ring = nic->tx_ring;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_tx;
    uint16_t ret = 0;
    uint16_t nb_sent = 0;;

    while(!force_quit) {
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
        ret = rte_eth_tx_burst(portid, 0, bufs, nb_tx);
        while(ret < nb_tx) {
            nb_tx -= ret;
            nb_sent += ret;
            ret = rte_eth_tx_burst(portid, 0, (bufs + nb_sent), nb_tx);
        }
    }
    
    nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    while(nb_tx != 0) {
        rte_pktmbuf_free_bulk(bufs, nb_tx);
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    }
    
    return 0;
}