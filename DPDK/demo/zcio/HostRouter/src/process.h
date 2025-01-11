#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "env.h"
#include "protocol.h"

#define MAX_BURST_NUM 32

struct mbufs {
    struct nic_info *input_nic;
    struct rte_mbuf **bufs;
    uint16_t num;
};

int phy_nic_receive(void *arg __rte_unused);
int phy_nic_send(void *arg __rte_unused);
int virtual_nic_receive(void *arg);
int virtual_nic_send(void *arg);

#endif // !__PROCESS_H__