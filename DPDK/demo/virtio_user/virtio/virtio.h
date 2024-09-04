#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include <rte_ether.h>
#include "common.h"

bool force_quit = false;

struct rte_ether_addr send_mac = {
    .addr_bytes = { 0x11, 0x11, 0x11, 0x22, 0x22, 0x22 }
};

// 初始化
int port_init(uint16_t port, struct rte_mempool *mbuf_pool);

// 数据包日志
void show_packet(FILE *log, struct rte_ether_addr src_mac, 
    struct rte_ether_addr dst_mac, uint16_t ether_type, 
    uint32_t src_ip, uint32_t dst_ip, char *msg);

#endif // !__VIRTIO_H__