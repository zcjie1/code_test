#include "process.h"
#include <byteswap.h>
#include <unistd.h>

extern struct config cfg;

struct nic_info* find_next_port(struct rte_mbuf *m)
{
    struct route_table *table = &cfg.rtable;
    struct rte_ether_hdr *eth = mbuf_eth_hdr(m);
    struct arphdr *arph = NULL;
    struct rte_ipv4_hdr *ipv4_hdr = NULL;
    uint32_t dstip = 0;
    // struct in_addr ip;
    
    if(eth->ether_type == htons(RTE_ETHER_TYPE_ARP)) {
        arph = mbuf_arphdr(m);
        dstip = arph->ar_tip;
    }else {
        ipv4_hdr = mbuf_ip_hdr(m);
        dstip = ipv4_hdr->dst_addr;
    }
    
    // if(eth->ether_type == htons(RTE_ETHER_TYPE_IPV4)) {
    //     ipv4_hdr = mbuf_ip_hdr(m);
    //     if(ipv4_hdr->next_proto_id == IPPROTO_UDP) {
    //         struct rte_udp_hdr *udp_hdr = mbuf_udp_hdr(m);
    //         printf("UDP Message src_port%d dst_port%d\n", ntohs(udp_hdr->src_port), ntohs(udp_hdr->dst_port));
    //     }
    // }
    
    
    // ip.s_addr = dstip;
    // printf("DSTIP Message for %s\n", inet_ntoa(ip));
    for(int i = 0; i < table->entry_num; i++) {
        if(dstip == table->entry[i].ipaddr) {
            // ipv4_hdr->dst_addr = table->entry[i].info->ipaddr;
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
    ip_addr.s_addr = (in_addr_t)cfg.phy_nic.info[0].ipaddr;
    uint32_t sip;
    uint32_t dip;
    struct rte_ether_addr src_mac;
    struct rte_ether_addr dst_mac;
    struct rte_ether_hdr *eth;
    
    uint16_t ret = 0;
    
    rte_eth_macaddr_get(cfg.phy_nic.info[0].portid, &src_mac);
    
    struct arphdr *arph = mbuf_arphdr(m);
    if(arph->ar_op == htons((uint16_t)ARP_REQUEST) && 
        arph->ar_tip == (uint32_t)ip_addr.s_addr) {
        eth = mbuf_eth_hdr(m);
        sip = arph->ar_tip;
        dip = arph->ar_sip;
        rte_ether_addr_copy(&eth->src_addr, &dst_mac);
        eth_hdr_set(eth, RTE_ETHER_TYPE_ARP, &src_mac, &dst_mac);
        arp_set_arphdr(arph, ARP_REPLY, sip, dip, &src_mac, &dst_mac);
        ret = rte_ring_enqueue(cfg.phy_nic.info[0].tx_ring, m);
        if(ret < 0) {
            rte_pktmbuf_free(m);
        }
    }else {
		rte_pktmbuf_free(m);
	}
}

static uint64_t recv_pkt_num;
int phy_nic_receive(void *arg __rte_unused)
{
    if(cfg.phy_nic.nic_num == 0)
        return 0;
    
    uint16_t portid = cfg.phy_nic.info[0].portid;
    // printf("Start receiving packet from port %u\n", portid);
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_rx;
    int ret = 0;
    
    struct rte_ether_hdr *eth_hdr = NULL;
    struct rte_ipv4_hdr *ipv4_hdr = NULL;
    
    while (!cfg.force_quit) {
        nb_rx = rte_eth_rx_burst(portid, 0, bufs, MAX_BURST_NUM);
        if (nb_rx == 0)
            continue;
        // printf("Receive %d packets from phy nic\n", nb_rx);
        for(int i = 0; i < nb_rx; i++) {
            eth_hdr = mbuf_eth_hdr(bufs[i]);
            if(eth_hdr->ether_type == htons(RTE_ETHER_TYPE_ARP)) {
                arp_process(bufs[i]);
            }else if(eth_hdr->ether_type == htons(RTE_ETHER_TYPE_IPV4)) {
                ipv4_hdr = mbuf_ip_hdr(bufs[i]);
                if(ipv4_hdr->next_proto_id != IPPROTO_UDP) {
                    rte_pktmbuf_free(bufs[i]);
                    continue;
                }
                
                struct nic_info *nic = find_next_port(bufs[i]);
                if(nic == NULL) {
                    rte_pktmbuf_free(bufs[i]);
                    continue;
                }
                ++recv_pkt_num;
                ret = rte_ring_enqueue(nic->tx_ring, bufs[i]);
                if(ret < 0) {
                    rte_pktmbuf_free(bufs[i]);
                    continue;
                }
                // printf("Receive %lu packet from phy nic\n", ++recv_pkt_num);
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
    if(eth_hdr->ether_type == htons(RTE_ETHER_TYPE_ARP))
        return;
    
    struct rte_ipv4_hdr *ipv4_hdr = mbuf_ip_hdr(m);
    struct rte_udp_hdr *udp_hdr = mbuf_udp_hdr(m);
    
    // UDP 端口互换
    rte_be16_t tmp_port = udp_hdr->src_port;
    udp_hdr->src_port = udp_hdr->dst_port;
    udp_hdr->dst_port = tmp_port;
    
    // IP 设置
    ipv4_hdr->dst_addr = ipv4_hdr->src_addr;
    ipv4_hdr->src_addr = cfg.phy_nic.info[0].ipaddr;
    ipv4_hdr->time_to_live = 64;
    
    // UDP 校验和
    udp_hdr->dgram_cksum = 0;
    udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ipv4_hdr, (void *)udp_hdr);
    
    // IP 校验和
    ipv4_hdr->hdr_checksum = 0;
    ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
    
    // MAC 地址交换
    struct rte_ether_addr tmp_mac;
    rte_ether_addr_copy(&eth_hdr->dst_addr, &tmp_mac);
    rte_ether_addr_copy(&eth_hdr->src_addr, &eth_hdr->dst_addr);
    rte_ether_addr_copy(&tmp_mac, &eth_hdr->src_addr);
}

static void loop_tx(uint16_t port_id, uint16_t queue_id, 
	struct rte_mbuf **tx_pkts, uint16_t nb_pkts)
{
	uint16_t ret = 0;
	uint16_t nb_sent = 0;

	ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts, nb_pkts);
	while(ret < nb_pkts && !cfg.force_quit) {
		nb_sent += ret;
		nb_pkts -= ret;
		ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts + nb_sent, nb_pkts);
	}
}

int phy_nic_send(void *arg __rte_unused)
{
    if(cfg.phy_nic.nic_num == 0)
        return 0;
    
    uint16_t portid = cfg.phy_nic.info[0].portid;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    struct rte_ring *tx_ring = cfg.phy_nic.info[0].tx_ring;
    uint16_t nb_tx = 0;
    uint16_t ret = 0;

    while(!cfg.force_quit) {
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
        if(nb_tx == 0)
            continue;
        // printf("Send %d packets from phy nic to outside\n", nb_tx);
        for(int i = 0; i < nb_tx; i++) {
            phynic_out_process(bufs[i]);
        }
        loop_tx(portid, 0, bufs, nb_tx);
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
    static uint64_t free_count;
    static uint64_t recv_count;
        
    while (!cfg.force_quit) {
        nb_rx = rte_eth_rx_burst(portid, 0, bufs, MAX_BURST_NUM);
        if (nb_rx == 0)
            continue;
        recv_count += nb_rx;
        // printf("Receive %lu packets from zcio client\n", recv_count);
        for(int i = 0; i < nb_rx; i++) {
            struct nic_info *info = find_next_port(bufs[i]);
            if(info == NULL) {
                printf("No route info for %lu packet\n", ++free_count);
                rte_pktmbuf_free(bufs[i]);
                continue;
            }
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

    while(!cfg.force_quit) {
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
        if(nb_tx == 0)
            continue;
        // printf("Send %d packets to zcio client\n", nb_tx);
        loop_tx(portid, 0, bufs, nb_tx);
    }
    
    nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    while(nb_tx != 0) {
        rte_pktmbuf_free_bulk(bufs, nb_tx);
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    }
    
    return 0;
}

int statistic_output(void *arg)
{
    while(!cfg.force_quit) {
        sleep(1);
        printf("Receive %lu packets from phy nic\n", recv_pkt_num);
    }
}