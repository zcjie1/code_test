#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <rte_dev.h>
#include <rte_bus.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_alarm.h>

#include "env.h"
#include "process.h"

extern struct config global_cfg;

// 信号处理函数
static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		global_cfg.force_quit = true;
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	uint16_t portid;
	unsigned int worker_id;
	
	ret = virtual_host_init(argc, argv);
	if (ret < 0)
		return -1;
		
	printf("\nStart Processing...\n\n");

	// assign worker
	for(int i = 0; i < global_cfg.virtual_nic.nic_num; i++) {
		global_cfg.curr_worker = rte_get_next_lcore(global_cfg.curr_worker, 1, 0);
		rte_eal_remote_launch(virtual_nic_send, &global_cfg.virtual_nic.info[i], global_cfg.curr_worker);
		global_cfg.curr_worker = rte_get_next_lcore(global_cfg.curr_worker, 1, 0);
		rte_eal_remote_launch(virtual_nic_receive, &global_cfg.virtual_nic.info[i], global_cfg.curr_worker);
		
	}
	global_cfg.curr_worker = rte_get_next_lcore(global_cfg.curr_worker, 1, 0);
	rte_eal_remote_launch(phy_nic_send, NULL, global_cfg.curr_worker);
	global_cfg.curr_worker = rte_get_next_lcore(global_cfg.curr_worker, 1, 0);
	rte_eal_remote_launch(phy_nic_receive, NULL, global_cfg.curr_worker);
	
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	rte_eal_mp_wait_lcore();

	virtual_host_destroy();
	rte_eal_cleanup();
	printf("Bye...\n");
	return 0;
}