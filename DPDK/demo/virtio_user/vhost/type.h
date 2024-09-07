#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdio.h>
#include <time.h>

// #define MBUF_BUF_SIZE 9600
#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 128
#define TX_SIZE 32

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 4096
#define TX_RING_SIZE 4096

#define PERIOD 2

#define SD_ETHER_TYPE_IPV4 ((uint16_t)0x0008) /**< IPv4 Protocol. */
#define MATCH_TYPE(x, y) ((x) == (y))

struct status {
    FILE *log;
	uint64_t total_rx_num;
	uint64_t total_tx_num;
	uint64_t real_rx_num;
	uint64_t real_tx_num;
	uint64_t rx_bytes;
	uint64_t tx_bytes;
	struct timespec start_time;
	struct timespec end_time;
};

typedef enum PORT_TYPE {
    RECV_PORT = 0,
    SEND_PORT,
}PORT_TYPE;

#endif // !__TYPE_H__
