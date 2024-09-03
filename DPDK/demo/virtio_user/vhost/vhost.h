#ifndef __VHOST_H__
#define __VHOST_H__

#include "type.h"

#define LOG_DIR "log"
#define LOG_FILE "log/vhost_statistic.log"

bool force_quit = false;

struct status statistics;

struct rte_ring *fwd_ring;

/* 日志 */

int generate_logdir(char *dir_path);
void show_stats(void *param);


/* 初始化 */

int port_init(uint16_t port, struct rte_mempool *mbuf_pool);
void ring_init(struct rte_ring *fwd_ring, char *name);

#endif // !__VHOST_H__