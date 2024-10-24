
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

#include "memctl.h"
#include "env.h"
#include "process.h"

extern struct config cfg;

// 信号处理函数
static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		cfg.force_quit = true;
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	uint16_t portid;
	unsigned int worker_id;
	
	ret = zcio_host_init(argc, argv);
	if (ret < 0)
		return -1;
		
	printf("\nStart Processing...\n\n");

	// 分配工作核心任务
	for(int i = 0; i < cfg.zcio_nic.nic_num; i++) {
		worker_id = rte_get_next_lcore(worker_id, 1, 0);
		rte_eal_remote_launch(zcio_nic_send, &cfg.zcio_nic.info[i], worker_id);
		worker_id = rte_get_next_lcore(worker_id, 1, 0);
		rte_eal_remote_launch(zcio_nic_receive, &cfg.zcio_nic.info[i], worker_id);
		
	}
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(phy_nic_send, NULL, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(phy_nic_receive, NULL, worker_id);
	// worker_id = rte_get_next_lcore(worker_id, 1, 0);
	// rte_eal_remote_launch(statistic_output, NULL, worker_id);
	
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

 out:
	// 等待工作核心结束任务
	rte_eal_mp_wait_lcore();

	// 关闭网卡
	RTE_ETH_FOREACH_DEV(portid) {
		printf("Closing port %d...\n", portid);
		ret = rte_eth_dev_stop(portid);
		if (ret != 0)
			printf("rte_eth_dev_stop: err=%d, port=%d\n",
			       ret, portid);
		rte_eth_dev_close(portid);
		printf("Done\n");
	}

	// 释放发送队列
	for(int i = 0; i < cfg.zcio_nic.nic_num; i++)
		nic_txring_release(&cfg.zcio_nic.info[i]);
	for(int i = 0; i < cfg.phy_nic.nic_num; i++)
		nic_txring_release(&cfg.phy_nic.info[i]);

	rte_mempool_free(cfg.mbuf_pool);

	// eal环境释放
	rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}