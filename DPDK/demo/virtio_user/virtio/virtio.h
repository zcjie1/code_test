#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include <rte_ether.h>
#include "common.h"

#define PERIOD 8

bool force_quit = false;

struct rte_ether_addr broadcast_mac = {
    .addr_bytes = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
};

// 初始化
int port_init(uint16_t port, struct rte_mempool *mbuf_pool);

// 数据包日志
void show_packet(FILE *log, struct rte_ether_addr src_mac, 
    struct rte_ether_addr dst_mac, uint16_t ether_type, 
    uint32_t src_ip, uint32_t dst_ip, char *msg);

void show_result(FILE *log);
void period_alarm_vhost(void *arg);

#endif // !__VIRTIO_H__