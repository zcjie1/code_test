#include "protocol.h"

extern struct config global_cfg;

static inline void 
eth_hdr_set(struct rte_ether_hdr *eth, uint16_t type, 
        struct rte_ether_addr *src_mac, struct rte_ether_addr *dst_mac)
{
    eth->ether_type = htons(type);
    rte_ether_addr_copy(src_mac, &eth->src_addr);
    rte_ether_addr_copy(dst_mac, &eth->dst_addr);
}

static inline void 
arp_set_arphdr(struct arphdr *arp, uint16_t op, uint32_t sip, uint32_t dip,
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

static int 
arp_ip2mac(uint32_t ip, struct rte_ether_addr *mac)
{
    struct rte_hash *arp_table = global_cfg.arp_table;

    int ret;
    uint32_t target_ip = ip;
    struct rte_ether_addr *tmp_mac;
    ret = rte_hash_lookup_data(arp_table, &target_ip, (void **)&tmp_mac);
    if(ret >= 0) {
        rte_ether_addr_copy(tmp_mac, mac);
        return ret;
    }

    if(ret == -ENOENT) {
        // send arp request
    }

    return ret;
}

static int 
neighbour_fill_eth_hdr(struct rte_mbuf *m, struct nic_info *if_input, uint16_t eth_type, 
    uint32_t target_ip, struct rte_ether_addr *target_mac)
{
    int ret = 0;
    struct nic_info *if_output;
    struct rte_ether_hdr *eth = mbuf_eth_hdr(m);
    struct rte_ether_addr src_mac;

    // map ipaddr into macaddr
    ret = arp_ip2mac(target_ip, target_mac);
    if(ret == -ENOENT) {
        /* no arp entry in arp_table, send arp request and reprocess the packet */
        rte_ring_enqueue(if_input->tx_ring, m);
    }else if(ret == -EINVAL) {
        perror("arp_table lookup error");
        goto err;
    }
    
    ret = rte_fib_lookup_bulk(global_cfg.fib, &target_ip, (uint64_t *)&if_output, 1);
    if(ret < 0)
        goto err;
    rte_eth_macaddr_get(if_output->portid, &src_mac);
    eth_hdr_set(eth, RTE_ETHER_TYPE_ARP, &src_mac, target_mac);
    return 0;

 err:
    return -1;
}

static void 
arp_table_add(uint32_t ip, struct rte_ether_addr *mac)
{
    struct rte_hash *arp_table = global_cfg.arp_table;
    
    uint32_t *ipaddr = malloc(sizeof(uint32_t));
    struct rte_ether_addr *macaddr = malloc(sizeof(struct rte_ether_addr));
    *ipaddr = ip;
    rte_ether_addr_copy(mac, macaddr);
    rte_hash_add_key_data(arp_table, (void *)ipaddr, (void *)macaddr);
}
void 
arp_process(struct rte_mbuf *m, struct nic_info *if_input)
{
	uint32_t my_ipaddr = rte_be_to_cpu_32(if_input->ipaddr);
    struct rte_hash *arp_table = global_cfg.arp_table;
    
    uint32_t new_sip;
    uint32_t new_dip;
    struct rte_ether_addr new_src_mac;
    struct rte_ether_addr new_dst_mac;
    struct rte_ether_hdr *eth;
    
    int ret = 0;
    uint32_t target_ip;
    struct nic_info *if_output;
    
    struct arphdr *arph = mbuf_arphdr(m);
    if(arph->ar_op == htons(ARP_REQUEST)) {
        eth = mbuf_eth_hdr(m);

        // add the sip and smac into arp_table
        arp_table_add(arph->ar_sip, &arph->ar_sha);

        // process the arp request
        if(arph->ar_sip == my_ipaddr) {
            new_sip = arph->ar_tip;
            new_dip = arph->ar_sip;
            rte_eth_macaddr_get(if_input->portid, &new_src_mac);
            rte_ether_addr_copy(&eth->src_addr, &new_dst_mac);
            arp_set_arphdr(arph, ARP_REPLY, new_sip, new_dip, &new_src_mac, &new_dst_mac);
            eth_hdr_set(eth, RTE_ETHER_TYPE_ARP, &new_src_mac, &new_dst_mac);
            ret = rte_ring_enqueue(if_input->tx_ring, m);
            if(ret < 0)
                goto free;
        }else {
            target_ip = ntohl(arph->ar_tip);
            ret = neighbour_fill_eth_hdr(m, if_input, RTE_ETHER_TYPE_ARP, target_ip, &new_dst_mac);
            if (ret < 0)
                goto free;
            ret = rte_ring_enqueue(if_output->tx_ring, m);
            if(ret < 0)
                goto free;
        }
        
    }else {
        goto free;
    }

    return;

 free:
    rte_pktmbuf_free(m);
}