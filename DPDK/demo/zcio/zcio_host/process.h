#ifndef __PROCESS_H__
#define __PROCESS_H__

int route_process(void *arg);
int phy_nic_receive(void *arg);
int phy_nic_send(void *arg);
int zcio_nic_receive(void *arg);
int zcio_nic_send(void *arg);

#endif // !__PROCESS_H__