/**
 * 程序功能概述：
 * - 初始化所有网卡（包括vhost网卡和物理网卡）
 * - 分配一个巨大的内存池
 * - 分配一个内存池管理模块，负责与服务容器通信，交换大页数据
 * - ...
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/fcntl.h>

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

#define MEMCTL_PATH "/tmp/memctl.sock"

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 512

#define RX_RING_NUM 1
#define TX_RING_NUM 1
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

bool force_quit = false;
uint64_t pkt_num = 0;

#define MAX_NIC_NUM 16
struct nic_info{
	int nic_num; // 网卡数量
	uint16_t portid[MAX_NIC_NUM]; // 网卡 port_id
};

static struct nic_info phy_nic;
static struct nic_info zcio_nic;
static struct rte_mempool *mbuf_pool;

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

static int
update_memory_region(const struct rte_memseg_list *msl __rte_unused,
		const struct rte_memseg *ms, void *arg)
{
	struct walk_arg *wa = arg;
	struct memory_region *mr;
	uint64_t start_addr, end_addr;
	size_t offset;
	int i, fd;

	fd = rte_memseg_get_fd_thread_unsafe(ms);
	if (fd < 0) {
		printf("Failed to get fd, ms=%p rte_errno=%d",
			ms, rte_errno);
		return -1;
	}

	if (rte_memseg_get_fd_offset_thread_unsafe(ms, &offset) < 0) {
		printf("Failed to get offset, ms=%p rte_errno=%d",
			ms, rte_errno);
		return -1;
	}

	start_addr = (uint64_t)(uintptr_t)ms->addr;
	end_addr = start_addr + ms->len;

	for (i = 0; i < wa->region_nr; i++) {
		if (wa->fds[i] != fd)
			continue;

		mr = &wa->regions[i];

		if (mr->host_start_addr + mr->memory_size < end_addr)
			mr->memory_size = end_addr - mr->host_start_addr;

		if (mr->host_start_addr > start_addr) {
			mr->host_start_addr = start_addr;
		}

		if (mr->mmap_offset > offset)
			mr->mmap_offset = offset;

		printf("index=%d fd=%d offset=0x%" PRIx64
			" addr=0x%" PRIx64 " len=%" PRIu64"\n", i, fd,
			mr->mmap_offset, mr->host_start_addr,
			mr->memory_size);

		return 0;
	}

	if (i >= 256) {
		printf("Too many memory regions");
		return -1;
	}

	mr = &wa->regions[i];
	wa->fds[i] = fd;

	mr->host_start_addr = start_addr;
	mr->memory_size = ms->len;
	mr->mmap_offset = offset;

	printf("index=%d fd=%d offset=0x%" PRIx64
		" addr=0x%" PRIx64 " len=%" PRIu64 "\n", i, fd,
		mr->mmap_offset, mr->host_start_addr,
		mr->memory_size);

	wa->region_nr++;

	return 0;
}

static int memory_manager(void *arg __rte_unused)
{
	struct walk_arg wa;
	rte_memseg_walk_thread_unsafe(update_memory_region, &wa);

	int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    struct msghdr msgh;
    struct iovec iov;
    struct cmsghdr *cmsg;
    size_t fd_size = wa.region_nr * sizeof(int);
	char ctrl[CMSG_SPACE(fd_size)];
	int ret;

     // 创建 AF_UNIX 套接字
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("MEMCTL: Socket creation failed");
        return -1;
    }

    // 清除旧的套接字文件
    unlink(MEMCTL_PATH);

    // 设置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, MEMCTL_PATH, sizeof(server_addr.sun_path) - 1);
	
	// 绑定套接字到地址
	ret = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("MEMCTL: Bind failed");
        close(server_sock);
        return -1;
    }

	// 将套接字设置为非阻塞模式
    if (fcntl(server_sock, F_SETFL, O_NONBLOCK) == -1) {
        perror("MEMCTL: Set non-blocking mode failed");
        close(server_sock);
        return -1;
    }

	// 开始监听
	ret = listen(server_sock, 8);
    if (ret == -1) {
        perror("MEMCTL: Listen failed");
        close(server_sock);
        return -1;
    }

	// 初始化iov结构
	iov.iov_base = (void *)&wa;
	iov.iov_len = sizeof(wa);

	// 初始化msghdr结构
	memset(&msgh, 0, sizeof(msgh));
	memset(ctrl, 0, sizeof(ctrl));
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_control = ctrl;
	msgh.msg_controllen = sizeof(ctrl);

	cmsg = CMSG_FIRSTHDR(&msgh);
	cmsg->cmsg_len = CMSG_LEN(fd_size);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	memcpy(CMSG_DATA(cmsg), wa.fds, fd_size);

	printf("MEMCTL: hugepage num: %d\n", wa.region_nr);
	for (int i = 0; i < wa.region_nr; i++) {
		printf("MEMCTL: fd=%d\n", wa.fds[i]);
		printf("	wa_host_phys_addr: %lx\n", wa.regions[i].host_start_addr);
		printf("	wa_memory_size: %lu\n", wa.regions[i].memory_size);
		printf("	wa_mmap_offset: %lu\n", wa.regions[i].mmap_offset);
 	}
	
	while(!force_quit) {
		// 接受客户端连接
		client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
		if (client_sock == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
                usleep(100);
            else
                perror("MEMCTL: Accept failed");
			continue;
		}

		// 发送消息
		ret = sendmsg(client_sock, &msgh, MSG_CMSG_CLOEXEC);
		while(ret == -1 && !force_quit) {
			perror("MEMCTL: Sendmsg failed");
			usleep(1000);
			ret = sendmsg(client_sock, &msgh, MSG_CMSG_CLOEXEC);
		}
		
		close(client_sock);
	}
	close(server_sock);
    unlink(MEMCTL_PATH);
}

// 网卡初始化
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf;
	struct rte_eth_dev_info dev_info;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	char device_name[256];

	struct rte_eth_rxconf rxconf;
	struct rte_eth_txconf txconf;
	
	printf("Initializing port %u... \n", port);
	fflush(stdout);

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	// 获取网卡信息
	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval < 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

	// 配置port_conf
	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
	if (dev_info.rx_offload_capa & RTE_ETH_RX_OFFLOAD_SCATTER)
		port_conf.rxmode.offloads |= RTE_ETH_RX_OFFLOAD_SCATTER;

	// 初始化网卡驱动
	retval = rte_eth_dev_configure(port, RX_RING_NUM, TX_RING_NUM, &port_conf);
	if (retval < 0) {
		printf("Cannot configure device: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		
	// 设置每个ring的描述符数量
	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval < 0) {
		printf("Cannot adjust number of descriptors: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		

	// 设置每个receive ring对应的内存池
	int port_socket = rte_eth_dev_socket_id(port);
	rxconf = dev_info.default_rxconf;
	rxconf.offloads = port_conf.rxmode.offloads;
	for (int r = 0; r < RX_RING_NUM; r++) {
		retval = rte_eth_rx_queue_setup(port, r, nb_rxd,
				port_socket, &rxconf, mbuf_pool);
		if (retval < 0) {
			printf("Cannot setup receive queue %u for port %u\n",
					r, port);
			return retval;
		}
			
	}

	// 配置发送队列
	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	for (int r = 0; r < TX_RING_NUM; r++) {
		retval = rte_eth_tx_queue_setup(port, r, nb_txd,
				port_socket, &txconf);
		if (retval < 0) {
			printf("Cannot setup transmit queue %u for port %u\n",
					r, port);
			return retval;
		}
			
	}

	retval = rte_eth_dev_set_ptypes(port, RTE_PTYPE_UNKNOWN, NULL, 0);
	if (retval < 0)
		printf("Port %u, Failed to disable Ptype parsing\n", port);

	// 启动网卡
	retval = rte_eth_dev_start(port);
	if (retval < 0) {
		printf("Cannot start device: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		
	
	retval = rte_eth_promiscuous_enable(port);
	if (retval < 0) {
		printf("Cannot enable promiscuous mode: err=%d, port=%u\n",
				retval, port);
		return retval;
	}
		

	// 输出网卡信息
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval < 0) {
		printf("Error reading MAC address for port %d\n", port);
		return retval;
	}

	printf("Port %u: \n", port);
    printf("    MAC: %02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 ":"
			   "%02"PRIx8 ":" "%02"PRIx8 ":" "%02"PRIx8 "\n",
			RTE_ETHER_ADDR_BYTES(&addr));
    
    rte_eth_dev_get_name_by_port(port, device_name);
    printf("    Device Name: %s\n", device_name);
    printf("    Driver Name: %s\n\n", dev_info.driver_name);

	// 统计网卡数量
	if(strcmp(dev_info.driver_name, "net_zcio") == 0) {
		zcio_nic.portid[zcio_nic.nic_num] = port;
		zcio_nic.nic_num++;
	}else {
		phy_nic.portid[phy_nic.nic_num] = port;
		phy_nic.nic_num++;
	}
	
	return 0;
}

// 信号处理函数
static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

static struct rte_mbuf* generate_testpkt(struct rte_mempool *mbuf_pool)
{
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);
	const struct rte_ether_addr src_mac = {
		.addr_bytes = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc }
	};
	const struct rte_ether_addr dst_mac = { 
		.addr_bytes = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } 
	};

	// Meta data 初始化
	pkt->data_len = 14 + 8; // MAC + Message
	pkt->data_off = RTE_PKTMBUF_HEADROOM;
	pkt->pkt_len = 14 + 8;
	pkt->nb_segs = 1;
	pkt->l2_len	= sizeof(struct rte_ether_hdr);
	pkt->l3_len	= 0;
	pkt->next = NULL;

	// 二层初始化
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
	rte_ether_addr_copy(&src_mac, &eth_hdr->src_addr);
	rte_ether_addr_copy(&dst_mac, &eth_hdr->dst_addr);
	// eth_hdr->ether_type = ((uint16_t)0x0008);
	eth_hdr->ether_type = rte_cpu_to_be_16((uint16_t)0x0800);
	uint64_t *data = (uint64_t *)(eth_hdr + 1);
	*data = rte_cpu_to_be_64((uint64_t)114514);
	
	return pkt;
}

static void parse_pkt(struct rte_mbuf *pkt)
{
	struct rte_ether_addr src_mac, dst_mac;
	uint16_t ether_type;
	
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
	src_mac = eth_hdr->src_addr;
	dst_mac = eth_hdr->dst_addr;
	ether_type = eth_hdr->ether_type;
	uint64_t *data_ptr = (uint64_t *)(eth_hdr + 1);
	uint64_t raw_data = *data_ptr;
	uint64_t data = rte_be_to_cpu_64(raw_data);
	printf("--------------------------PACKET %lu--------------------------\n", pkt_num++);
	printf("src mac %02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 "\n",
		RTE_ETHER_ADDR_BYTES(&src_mac));
	printf("dst mac %02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 ":%02"PRIx8 "\n",
		RTE_ETHER_ADDR_BYTES(&dst_mac));
	printf("ether type %04"PRIx16 "\n", ether_type);
	printf("raw data %016"PRIx64 "\n", raw_data);
	printf("data %016"PRIu64 "\n", data);
	printf("--------------------------------------------------------------\n\n");
}

static void loop_tx(uint16_t port, struct rte_mbuf **pkt, uint16_t num)
{
	int ret;
	ret = rte_eth_tx_burst(port, 0, pkt, num);
	while(!ret && !force_quit) {
		ret = rte_eth_tx_burst(port, 0, pkt, num);
		printf("Port%u: retry to tx_burst\n", port);
		sleep(5);
	}
}

static void loop_rx(uint16_t port, struct rte_mbuf **pkt, uint16_t num)
{
	int ret;
	ret = rte_eth_rx_burst(port, 0, pkt, num);
	while(!ret && !force_quit) {
		ret = rte_eth_rx_burst(port, 0, pkt, num);
		// printf("Port%u: retry to rx_burst\n", port);
		usleep(100);
	}
}

static int server_test(void *arg)
{
	uint16_t *portid = (uint16_t *)arg;
	
	int ret = 0;
	struct rte_mbuf *pkt = generate_testpkt(mbuf_pool);
	while(!force_quit) {
		loop_tx(portid[0], &pkt, 1);
		loop_rx(portid[0], &pkt, 1);
		parse_pkt(pkt);
		loop_tx(portid[1], &pkt, 1);
		loop_rx(portid[1], &pkt, 1);
		parse_pkt(pkt);
		usleep(1000);
	}
	
	rte_pktmbuf_free(pkt);
	return 0;
}

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
	if (nb_lcores < 3)
		rte_exit(EXIT_FAILURE, "Error: The number of work cores is insufficient\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("ports number: %u\n", nb_ports);
	if (nb_ports < 2)
		rte_exit(EXIT_FAILURE, "Error: The number of ports is insufficient\n");

    // 分配内存池
	mbuf_pool = rte_pktmbuf_pool_create("share_pool", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	
	// 分配工作核心任务
	worker_id = rte_get_next_lcore(-1, 1, 0);
	rte_eal_remote_launch(memory_manager, NULL, worker_id);

    // 初始化网卡
	RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid, mbuf_pool) != 0) {
			force_quit = true;
			printf("\nError: Fail to init port %"PRIu16"\n", portid);
			goto out;
		}
		
	printf("\nStart Processing...\n\n");

	if(zcio_nic.nic_num < 2)
		rte_exit(EXIT_FAILURE, "Error: The number of zcio nic is not enough\n");

	// 分配工作核心任务
	worker_id = rte_get_next_lcore(worker_id, 1, 0);
	rte_eal_remote_launch(server_test, zcio_nic.portid, worker_id);
	
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

	// eal环境释放
	rte_eal_cleanup();
	printf("Bye...\n");

	return 0;
}