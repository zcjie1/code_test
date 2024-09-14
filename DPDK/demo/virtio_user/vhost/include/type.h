#ifndef __TYPE_H__
#define __TYPE_H__

#include <time.h>
#include "utils.h"

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
