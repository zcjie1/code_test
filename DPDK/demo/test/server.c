#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_dev.h>
#include <rte_bus.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#define SOCKET_PATH "/tmp/af_unix.socket"

bool force_quit = false;

struct memory_region {
	uint64_t guest_phys_addr;
	uint64_t memory_size;
	uint64_t userspace_addr;
	uint64_t mmap_offset;
};

struct walk_arg {
	int fds[64];
	int region_nr;
    struct memory_region regions[256];
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

		if (mr->userspace_addr + mr->memory_size < end_addr)
			mr->memory_size = end_addr - mr->userspace_addr;

		if (mr->userspace_addr > start_addr) {
			mr->userspace_addr = start_addr;
			mr->guest_phys_addr = start_addr;
		}

		if (mr->mmap_offset > offset)
			mr->mmap_offset = offset;

		printf("index=%d fd=%d offset=0x%" PRIx64
			" addr=0x%" PRIx64 " len=%" PRIu64"\n", i, fd,
			mr->mmap_offset, mr->userspace_addr,
			mr->memory_size);

		return 0;
	}

	if (i >= 256) {
		printf("Too many memory regions");
		return -1;
	}

	mr = &wa->regions[i];
	wa->fds[i] = fd;

	mr->guest_phys_addr = start_addr;
	mr->userspace_addr = start_addr;
	mr->memory_size = ms->len;
	mr->mmap_offset = offset;

	printf("index=%d fd=%d offset=0x%" PRIx64
		" addr=0x%" PRIx64 " len=%" PRIu64 "\n", i, fd,
		mr->mmap_offset, mr->userspace_addr,
		mr->memory_size);

	wa->region_nr++;

	return 0;
}

int main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	unsigned int nb_ports;
	unsigned int nb_lcores;

    // eal环境初始化
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	// 工作核心数量
	nb_lcores = rte_lcore_count();
	if (nb_lcores < 1)
		rte_exit(EXIT_FAILURE, "Error: The number of work cores is insufficient\n");

    // 网卡数量
	nb_ports = rte_eth_dev_count_avail();
	printf("ports number: %u\n", nb_ports);

    // 分配内存池
	mbuf_pool = rte_pktmbuf_pool_create("share_pool", 4096 * 5,
		512, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    
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

     // 创建 AF_UNIX 套接字
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 清除旧的套接字文件
    unlink(SOCKET_PATH);

    // 设置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 绑定套接字到地址
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(server_sock, 5) == -1) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

	// 接受客户端连接
	client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
	if (client_sock == -1) {
		perror("Accept failed");
		goto err;
	}

	// 初始化iov结构
	iov.iov_base = (uint8_t *)&wa;
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

	// 发送消息
	if (sendmsg(client_sock, &msgh, MSG_CMSG_CLOEXEC) == -1)
		perror("Sendmsg failed");

err:
	// 关闭连接
	close(client_sock);

	rte_mempool_free(mbuf_pool);

	// eal环境释放
	rte_eal_cleanup();

	return 0;
}