#ifndef __ENV_H__
#define __ENV_H__

#include <stdint.h>
#include "log.h"
#include "type.h"
#include "protocol.h"

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 128
#define TX_SIZE 32
#define DEQUEUE_ONCE 8

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 4096
#define TX_RING_SIZE 4096

void vhost_init(int argc, char **argv);
void vhost_exit(void);

#endif // !__ENV_H__