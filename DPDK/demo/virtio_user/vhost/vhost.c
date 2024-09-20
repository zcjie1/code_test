/**
 * 程序功能概述：
 * - 1个主控核心 + 2个工作核心
 * - 主控核心实现环境初始化、内存分配等功能
 * - 工作核心1 发送数据包，从发送队列中取出数据包，更新统计信息，发送至virtio端
 * - 工作核心2 接收数据包，更新统计信息，并放入发送队列
 */

#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_dev.h>
#include <rte_bus.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_alarm.h>
#include <rte_vhost.h>

#include "vhost.h"
#include "env.h"

extern struct status statistics;
extern struct rte_ring *fwd_ring;

bool force_quit = false;
bool start_ready = false;
uint16_t recv_port = 0;
uint16_t send_port = 1;

// 接收数据包，放入fwd_ring
static int phy_recv_pkt(void *arg)
{
	uint16_t port = *( (uint16_t *)arg );
	struct rte_mbuf *bufs[BURST_SIZE];
	unsigned int free_space;
	int nb_rx = 0;

	while(!start_ready) {
		if(force_quit)
			return 0;
		usleep(10);
	}

	printf("Start Receiving...\n");
	while(!force_quit) {
		nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

		if (unlikely(nb_rx == 0))
			continue;
		statistics.total_rx_num += nb_rx;

		int ret = rte_ring_enqueue_bulk(fwd_ring, (void **)bufs, nb_rx, &free_space);
		statistics.real_rx_num += ret;
		if(ret == 0) {
			for (int i = 0; i < free_space; i++) {
				if (rte_ring_enqueue(fwd_ring, bufs[i]) < 0) {
					rte_pktmbuf_free(bufs[i]);
					continue;
				}
				statistics.real_rx_num += 1;
				statistics.rx_bytes += rte_pktmbuf_pkt_len(bufs[i]);
			}
		}else {
			for (int i = 0; i < nb_rx; i++) {
				statistics.rx_bytes += rte_pktmbuf_pkt_len(bufs[i]);
			}
		}
	}

	return 0;
}

// 从fwd_ring取出数据包，发送至virtio端
static int vhost_send_pkt(void *arg)
{
	uint16_t port = *( (uint16_t *)arg );
	struct rte_mbuf *vhost_rx_buf;
	struct rte_mbuf *bufs[TX_SIZE];
	
	int ret = 0;
	uint16_t nb_rx = 0;
	uint16_t nb_tx = 0;
	int burst = 0;

	while(!start_ready) {
		if(force_quit)
			return 0;
		usleep(10);
	}

	printf("Start Sending...\n");
	while(!force_quit) {

		// 接收virtio端发送的定时消息并转发回去
		nb_rx = rte_eth_rx_burst(port, 0, &vhost_rx_buf, 1);
		if(nb_rx) {
			nb_tx = rte_eth_tx_burst(port, 0, &vhost_rx_buf, 1);
			if (unlikely(nb_tx < 1)) {
				rte_pktmbuf_free(vhost_rx_buf);
				printf("Fail to send back one packet from virtio\n");
			}
		}

		// 从队列中获取待发送数据包
		burst = 0;
		while(burst < TX_SIZE) {
			ret = rte_ring_dequeue_bulk(fwd_ring, (void **)(bufs + burst), DEQUEUE_ONCE, NULL);
			if(ret == 0)
				break;
			burst += ret;
		}
		if(!burst) {
			usleep(100);
			continue;
		}
			
		// 统计数据包信息
		statistics.total_tx_num += burst;
		for(int i = 0; i < burst; i++)
			statistics.tx_bytes += rte_pktmbuf_pkt_len(bufs[i]);
		
		// 发送数据包
		nb_tx = rte_eth_tx_burst(port, 0, bufs, burst);
		statistics.real_tx_num += nb_tx;
		if (unlikely(nb_tx < burst)) {
			for (uint16_t i = nb_tx; i < burst; i++) {
				statistics.tx_bytes -= rte_pktmbuf_pkt_len(bufs[i]);
				rte_pktmbuf_free(bufs[i]);
			}
		}
	}

	while(rte_ring_dequeue(fwd_ring, (void **)&vhost_rx_buf) == 0)
		rte_pktmbuf_free(vhost_rx_buf);

	return 0;
}

int main(int argc, char *argv[]) {

	vhost_init(argc, argv);

    printf("\nStart Processing...\n\n");

    // 分配工作核心任务
    unsigned int worker_id = -1;
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(vhost_send_pkt, &send_port, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(phy_recv_pkt, &recv_port, worker_id);

	// 等待工作核心结束任务
	rte_eal_mp_wait_lcore();

	vhost_exit();
	printf("Bye...\n");

	return 0;
}