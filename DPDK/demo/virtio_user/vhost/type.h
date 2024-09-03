#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdio.h>

#define NUM_MBUFS 4096
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 32

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 2048
#define TX_RING_SIZE 2048

#define PERIOD 5

struct status {
    FILE *log;
	uint64_t total_rx;
	uint64_t total_tx;
	uint64_t real_rx;
	uint64_t real_tx;
};

typedef enum PORT_TYPE {
    RECV_PORT = 0,
    SEND_PORT,
}PORT_TYPE;

#endif // !__TYPE_H__
