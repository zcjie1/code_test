#include "process.h"
#include <byteswap.h>
#include <unistd.h>

extern struct config global_cfg;
static void process_packets(struct mbufs *packets)
{
    struct rte_ether_hdr *eth_hdr;
    uint16_t nb_pkts = packets->num;
    struct rte_mbuf **bufs = packets->bufs;
    struct nic_info *input_nic = packets->input_nic;

    for(int i = 0; i < nb_pkts; i++) {
        eth_hdr = mbuf_eth_hdr(bufs[i]);
        if(eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP))
            arp_process(bufs[i], input_nic);
    }
}

int phy_nic_receive(void *arg __rte_unused)
{
    if(global_cfg.phy_nic.nic_num == 0)
        return 0;
    
    uint16_t portid = global_cfg.phy_nic.info[0].portid;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_rx;
    struct mbufs packets = {&global_cfg.phy_nic.info[0], bufs, 0};
    
    while (!global_cfg.force_quit) {
        nb_rx = rte_eth_rx_burst(portid, 0, bufs, MAX_BURST_NUM);
        if (nb_rx == 0)
            continue;
        packets.num = nb_rx;
        process_packets(&packets);
    }
    return 0;
}

int virtual_nic_receive(void *arg)
{
    struct nic_info *nic = arg;
    uint16_t portid = nic->portid;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_rx;
    struct mbufs packets = {nic, bufs, 0};
        
    while (!global_cfg.force_quit) {
        nb_rx = rte_eth_rx_burst(portid, 0, bufs, MAX_BURST_NUM);
        if (nb_rx == 0)
            continue;
        packets.num = nb_rx;
        process_packets(&packets);
    }
}

static void send_burst(uint16_t port_id, uint16_t queue_id, 
	struct rte_mbuf **tx_pkts, uint16_t nb_pkts)
{
	uint16_t ret = 0;
	uint16_t nb_sent = 0;
    uint16_t count = 0;

	ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts, nb_pkts);
	while(ret < nb_pkts && !global_cfg.force_quit && count < 4) {
        count++;
		nb_sent += ret;
		nb_pkts -= ret;
		ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts + nb_sent, nb_pkts);
	}

    rte_pktmbuf_free_bulk(tx_pkts + nb_sent + ret, nb_pkts - ret);
}

int phy_nic_send(void *arg __rte_unused)
{
    if(global_cfg.phy_nic.nic_num == 0)
        return 0;
    
    uint16_t portid = global_cfg.phy_nic.info[0].portid;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    struct rte_ring *tx_ring = global_cfg.phy_nic.info[0].tx_ring;
    uint16_t nb_tx = 0;

    while(!global_cfg.force_quit) {
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
        if(nb_tx == 0)
            continue;
        send_burst(portid, 0, bufs, nb_tx);
    }

    // release the remained packets
    nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    while(nb_tx != 0) {
        rte_pktmbuf_free_bulk(bufs, nb_tx);
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    }
    
    return 0;
}

int virtual_nic_send(void *arg)
{
    struct nic_info *nic = arg;
    uint16_t portid = nic->portid;
    struct rte_ring *tx_ring = nic->tx_ring;
    struct rte_mbuf *bufs[MAX_BURST_NUM];
    uint16_t nb_tx = 0;
    
    while(!global_cfg.force_quit) {
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
        if(nb_tx == 0)
            continue;
        send_burst(portid, 0, bufs, nb_tx);
    }
    
    // release the remained packets
    nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    while(nb_tx != 0) {
        rte_pktmbuf_free_bulk(bufs, nb_tx);
        nb_tx = rte_ring_dequeue_burst(tx_ring, (void **)bufs, MAX_BURST_NUM, NULL);
    }
    
    return 0;
}