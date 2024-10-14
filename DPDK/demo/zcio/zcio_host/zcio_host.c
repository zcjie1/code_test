
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

bool force_quit = false;
struct nic phy_nic;
struct nic zcio_nic;
struct route_table rtable;

struct rte_mempool *mbuf_pool;
struct rte_ring *route_ring;

// 信号处理函数
static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

// static struct rte_mbuf* generate_testpkt(struct rte_mempool *mbuf_pool)
// {
// 	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);
// 	const struct rte_ether_addr src_mac = {
// 		.addr_bytes = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc }
// 	};
// 	const struct rte_ether_addr dst_mac = { 
// 		.addr_bytes = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } 
// 	};

// 	// Meta data 初始化
// 	pkt->data_len = 14 + 8; // MAC + Message
// 	pkt->data_off = RTE_PKTMBUF_HEADROOM;
// 	pkt->pkt_len = 14 + 8;
// 	pkt->nb_segs = 1;
// 	pkt->l2_len	= sizeof(struct rte_ether_hdr);
// 	pkt->l3_len	= 0;
// 	pkt->next = NULL;

// 	// 二层初始化
// 	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
// 	rte_ether_addr_copy(&src_mac, &eth_hdr->src_addr);
// 	rte_ether_addr_copy(&dst_mac, &eth_hdr->dst_addr);
// 	// eth_hdr->ether_type = ((uint16_t)0x0008);
// 	eth_hdr->ether_type = rte_cpu_to_be_16((uint16_t)0x0800);
// 	uint64_t *data = (uint64_t *)(eth_hdr + 1);
// 	*data = rte_cpu_to_be_64((uint64_t)114514);
	
// 	return pkt;
// }

// static void parse_pkt(struct rte_mbuf *pkt)
// {
// 	struct rte_ether_addr src_mac, dst_mac;
// 	uint16_t ether_type;
	
// 	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
// 	src_mac = eth_hdr->src_addr;
// 	dst_mac = eth_hdr->dst_addr;
// 	ether_type = eth_hdr->ether_type;
// 	uint64_t *data_ptr = (uint64_t *)(eth_hdr + 1);
// 	uint64_t raw_data = *data_ptr;
// 	uint64_t data = rte_be_to_cpu_64(raw_data);
// 	printf("--------------------------PACKET %lu--------------------------\n", pkt_num++);
// 	printf("src mac %02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 "\n",
// 		RTE_ETHER_ADDR_BYTES(&src_mac));
// 	printf("dst mac %02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 "\n",
// 		RTE_ETHER_ADDR_BYTES(&dst_mac));
// 	printf("ether type %04"PRIx16 "\n", ether_type);
// 	printf("raw data %016"PRIx64 "\n", raw_data);
// 	printf("data %016"PRIu64 "\n", data);
// 	printf("--------------------------------------------------------------\n\n");
// }

// static void loop_tx(uint16_t port, struct rte_mbuf **pkt, uint16_t num)
// {
// 	int ret;
// 	ret = rte_eth_tx_burst(port, 0, pkt, num);
// 	while(!ret && !force_quit) {
// 		ret = rte_eth_tx_burst(port, 0, pkt, num);
// 		printf("Port%u: retry to tx_burst\n", port);
// 		sleep(5);
// 	}
// }

// static void loop_rx(uint16_t port, struct rte_mbuf **pkt, uint16_t num)
// {
// 	int ret;
// 	ret = rte_eth_rx_burst(port, 0, pkt, num);
// 	while(!ret && !force_quit) {
// 		ret = rte_eth_rx_burst(port, 0, pkt, num);
// 		// printf("Port%u: retry to rx_burst\n", port);
// 		usleep(100);
// 	}
// }

// static int server_test(void *arg)
// {
// 	uint16_t *portid = (uint16_t *)arg;
	
// 	int ret = 0;
// 	struct rte_mbuf *pkt = generate_testpkt(mbuf_pool);
// 	while(!force_quit) {
// 		loop_tx(portid[0], &pkt, 1);
// 		loop_rx(portid[0], &pkt, 1);
// 		parse_pkt(pkt);
// 		loop_tx(portid[1], &pkt, 1);
// 		loop_rx(portid[1], &pkt, 1);
// 		parse_pkt(pkt);
// 		usleep(1000);
// 	}
	
// 	rte_pktmbuf_free(pkt);
// 	return 0;
// }

int main(int argc, char *argv[])
{
	unsigned int nb_ports;
	unsigned int nb_lcores;
	uint16_t portid;
	unsigned int worker_id;

	force_quit = false;

    // eal环境初始化
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	// 工作核心数量
	nb_lcores = rte_lcore_count();
	if (nb_lcores < 12)
		rte_exit(EXIT_FAILURE, "Error: The number of work cores is insufficient\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("ports number: %u\n", nb_ports);
	if (nb_ports < 4)
		rte_exit(EXIT_FAILURE, "Error: The number of ports is insufficient\n");

    // 分配内存池
	mbuf_pool = rte_pktmbuf_pool_create("share_pool", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	
	// 内存共享管理
	worker_id = rte_get_next_lcore(-1, 1, 0);
	rte_eal_remote_launch(memory_manager, NULL, worker_id);

    // 初始化网卡
	RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid, mbuf_pool) != 0) {
			force_quit = true;
			printf("\nError: Fail to init port %"PRIu16"\n", portid);
			goto out;
		}
	
	// route_ring = rte_ring_create("route_ring", 4096, rte_socket_id(), RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);
	route_table_init();
		
	printf("\nStart Processing...\n\n");

	if(zcio_nic.nic_num < 3)
		rte_exit(EXIT_FAILURE, "Error: The number of zcio nic is not enough\n");

	// 分配工作核心任务
	// worker_id = rte_get_next_lcore(worker_id, 1, 0);
	// rte_eal_remote_launch(route_process, NULL, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(phy_nic_receive, NULL, worker_id);
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(phy_nic_send, NULL, worker_id);
	for(int i = 0; i < zcio_nic.nic_num; i++) {
		worker_id = rte_get_next_lcore(worker_id, 1, 0);
		rte_eal_remote_launch(zcio_nic_receive, &zcio_nic.info[i], worker_id);
		worker_id = rte_get_next_lcore(worker_id, 1, 0);
		rte_eal_remote_launch(zcio_nic_send, &zcio_nic.info[i], worker_id);
	}
	
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

	// 释放发送队列和路由队列
	rte_ring_free(route_ring);
	for(int i = 0; i < zcio_nic.nic_num; i++)
		nic_txring_release(&zcio_nic.info[i]);
	for(int i = 0; i < phy_nic.nic_num; i++)
		nic_txring_release(&phy_nic.info[i]);

	rte_mempool_free(mbuf_pool);

	// eal环境释放
	rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}