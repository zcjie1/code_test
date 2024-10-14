#ifndef __MEMCTL_H__
#define __MEMCTL_H__

#include <stdint.h>
#include <rte_mbuf.h>

#define MEMCTL_PATH "/tmp/memctl.sock"

struct memory_region {
	uint64_t host_start_addr;
	uint64_t memory_size;
	uint64_t mmap_offset;
};

#define MAX_FD_NUM 64
#define MAX_REGION_NUM 64
struct walk_arg {
	int region_nr;
	int fds[MAX_FD_NUM];
    struct memory_region regions[MAX_REGION_NUM];
};

int memory_manager(void *arg __rte_unused);

#endif // !__MEMCTL_H__