#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <rte_errno.h>
#include <unistd.h>
#include "memctl.h"
#include "env.h"

// extern bool force_quit;
extern struct config cfg;

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
int memory_manager(void *arg __rte_unused)
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
	
	while(!cfg.force_quit) {
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
		while(ret == -1 && !cfg.force_quit) {
			perror("MEMCTL: Sendmsg failed");
			usleep(1000);
			ret = sendmsg(client_sock, &msgh, MSG_CMSG_CLOEXEC);
		}
		
		close(client_sock);
	}
	close(server_sock);
    unlink(MEMCTL_PATH);
}