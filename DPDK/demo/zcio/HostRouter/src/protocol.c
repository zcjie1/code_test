#include "protocol.h"

extern struct config global_cfg;

/**
 * @brief fill the ethernet header
 * @param eth ethernet header
 * @param type ethernet type
 * @param src_mac source mac
 * @param dst_mac destination mac
 * @return void
 */
static inline void 
eth_hdr_set(struct rte_ether_hdr *eth, uint16_t type, 
        struct rte_ether_addr *src_mac, struct rte_ether_addr *dst_mac)
{
    eth->ether_type = htons(type);
    rte_ether_addr_copy(src_mac, &eth->src_addr);
    rte_ether_addr_copy(dst_mac, &eth->dst_addr);
}

/**
 * @brief fill the arp header
 * @param arp arp header
 * @param op arp operation code
 * @param sip host-endian source ip
 * @param dip host-endian destination ip
 * @param sa source mac
 * @param da destination mac
 * @return void
 */
static inline void 
arp_set_arphdr(struct arphdr *arp, uint16_t op, uint32_t sip, uint32_t dip,
    struct rte_ether_addr *sa, struct rte_ether_addr *da)
{
    arp->ar_hrd = htons(0x01);
    arp->ar_pro = htons(0x0800);
    arp->ar_hln = 0x06;
    arp->ar_pln = 0x04;
    arp->ar_op = htons(op);
    arp->ar_sip = htobe32(sip);
    arp->ar_tip = htobe32(dip);
    rte_ether_addr_copy(sa, &arp->ar_sha);
    rte_ether_addr_copy(da, &arp->ar_tha);
}

/**
 * @brief send arp request to target ip
 * @param if_output output nic
 * @param target_ip host-endian target ip
 * @return void
 */
static void
send_arp_request(struct nic_info *if_output, uint32_t target_ip)
{
    struct rte_mempool *mbuf_pool = global_cfg.mbuf_pool;
    struct rte_ether_addr broadcast_addr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    int ret = 0;
    struct rte_mbuf *arp_request;
    struct rte_ether_hdr *eth;
    struct arphdr *arp_hdr;
    struct rte_ether_addr src_mac;
    struct rte_ether_addr dst_mac;
    uint32_t sip, tip;
    struct rte_ether_addr src_mac, dst_mac;

    if(target_ip == if_output->ipaddr)
        return;
    arp_request = rte_pktmbuf_alloc(mbuf_pool);
    arp_hdr = mbuf_arphdr(arp_request);
    eth = mbuf_eth_hdr(arp_request);
    sip = if_output->ipaddr;
    tip = target_ip;
    rte_eth_macaddr_get(if_output->portid, &src_mac);
    memset((void *)&dst_mac, 0, sizeof(dst_mac));
    arp_set_arphdr(arp_hdr, ARP_REQUEST, sip, tip, &src_mac, &dst_mac);
    eth_hdr_set(eth, RTE_ETHER_TYPE_ARP, &src_mac, &broadcast_addr);
    ret = rte_ring_enqueue(if_output->tx_ring, arp_request);
    while(ret < 0)
        ret = rte_ring_enqueue(if_output->tx_ring, arp_request);
}

/**
 * @brief lookup the mac address in arp table
 * @param ip host-endian ip
 * @param mac mac address
 * @return int
 */
static int 
arp_ip2mac(uint32_t ip, struct rte_ether_addr *mac)
{
    struct rte_hash *arp_table = global_cfg.arp_table;
    struct nic *vnic = &global_cfg.virtual_nic;
    struct nic *pnic = &global_cfg.phy_nic;

    int ret;
    uint32_t target_ip = ip;
    struct rte_ether_addr *tmp_mac;
    ret = rte_hash_lookup_data(arp_table, &target_ip, (void **)&tmp_mac);
    if(ret >= 0) {
        rte_ether_addr_copy(tmp_mac, mac);
        return ret;
    }

    if(ret == -ENOENT) {
        // broadcast arp request
        for(int i = 0; i < vnic->nic_num; i++)
            send_arp_request(&vnic->info[i], target_ip);
        for(int i = 0; i < pnic->nic_num; i++)
            send_arp_request(&pnic->info[i], target_ip);
        return -ENOENT;
    }

    return ret;
}

/**
 * @brief lookup and fill dest mac of ethernet header
 * @param m packet buffer
 * @param if_input input nic
 * @param eth_type ethernet type
 * @param target_ip host-endian target ip
 * @param target_mac target mac
 */
static int 
neighbour_fill_eth_hdr(struct rte_mbuf *m, struct nic_info *if_input, uint16_t eth_type, 
    uint32_t target_ip, struct rte_ether_addr *target_mac)
{
    int ret = 0;
    struct nic_info *if_output;
    struct rte_ether_hdr *eth = mbuf_eth_hdr(m);
    struct rte_ether_addr src_mac;

 ip2mac:
    // map ipaddr into macaddr
    ret = arp_ip2mac(target_ip, target_mac);
    if(ret == -ENOENT) {
        /* no arp entry in arp_table, send arp request and reprocess the packet */
        rte_delay_us_sleep(10);
        goto ip2mac;
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

/**
 * @brief add the ipaddr and macaddr into arp_table
 * @param ip host-endian ipaddr
 * @param mac macaddr
 * @return void
 */
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
	uint32_t in_ipaddr = if_input->ipaddr;
    struct rte_hash *arp_table = global_cfg.arp_table;
    struct nic *vnic = &global_cfg.virtual_nic;
    struct nic *pnic = &global_cfg.phy_nic;
    
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
        arp_table_add(ntohl(arph->ar_sip), &arph->ar_sha);

        // if arp->tip != if_input's ipaddr
        if(arph->ar_tip != htobe32(in_ipaddr))
            goto free;

        // fill the arp header
        new_sip = in_ipaddr;
        new_dip = ntohl(arph->ar_sip);
        rte_eth_macaddr_get(if_input->portid, &new_src_mac);
        rte_ether_addr_copy(&arph->ar_sha, &new_dst_mac);
        arp_set_arphdr(arph, ARP_REPLY, new_sip, new_dip, &new_src_mac, &new_dst_mac);

        // fill ether header
        rte_ether_addr_copy(&eth->src_addr, &new_dst_mac);
        eth_hdr_set(eth, RTE_ETHER_TYPE_ARP, &new_src_mac, &new_dst_mac);

        // send the reply
        ret = rte_ring_enqueue(if_input->tx_ring, m);
        if(ret < 0)
            goto free;
        
    }else if (arph->ar_op == htons(ARP_REPLY)) {
        arp_table_add(ntohl(arph->ar_sip), &arph->ar_sha);
        goto free;
    }else {
        perror("unknown arp op");
        goto free;
    }
    
    return;

 free:
    rte_pktmbuf_free(m);
    return;
}