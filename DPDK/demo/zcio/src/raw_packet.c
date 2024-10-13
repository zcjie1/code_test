#include <rte_ethdev.h>
#include "raw_packet.h"

#define ALLOC_NUM(x) (((x) > 32768) ? 0 : ((x) + 32767))
#define FREE_MAGIC_NUM 114514

int
raw_packet_alloc_server(struct rte_mempool *mbuf_pool, 
        struct rte_mbuf **mbufs, unsigned int count)
{
    return rte_pktmbuf_alloc_bulk(mbuf_pool, mbufs, count);
}

void
raw_packet_free_server(struct rte_mbuf **m, unsigned int count)
{   
    rte_pktmbuf_free_bulk(m, count);
}


int
raw_packet_alloc_client(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **rx_pkts, const uint16_t nb_pkts)
{
	return rte_eth_rx_burst(port_id, queue_id, rx_pkts, ALLOC_NUM(nb_pkts));
}

void
raw_packet_free_client(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **tx_pkts, const uint16_t nb_pkts)
{
    if(tx_pkts == NULL)
        return;
    
    for(int i = 0; i < nb_pkts; i++) {
        tx_pkts[i]->dynfield1[0] = FREE_MAGIC_NUM;
    }
    
    int ret = 0;
    int avail_num = nb_pkts;
    int sent_num = 0;
    while(avail_num > 0) {
        ret = rte_eth_tx_burst(port_id, queue_id, tx_pkts + sent_num, avail_num);
        avail_num -= ret;
        sent_num += ret;
    }
}

int 
raw_packet_send(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **tx_pkts, const uint16_t nb_pkts)
{
    return rte_eth_tx_burst(port_id, queue_id, tx_pkts, nb_pkts);
}

int 
raw_packet_recv(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **rx_pkts, const uint16_t nb_pkts)
{
    return rte_eth_rx_burst(port_id, queue_id, rx_pkts, nb_pkts);
}