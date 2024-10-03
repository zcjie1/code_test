#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/mman.h>

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_dev.h>
#include <rte_bus.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_dev.h>

#define SOCKET_PATH "/tmp/af_unix.socket"

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
    void *data;
};

void show_packet(struct rte_ether_addr src_mac, 
    struct rte_ether_addr dst_mac, uint16_t ether_type)
{
	char smac[RTE_ETHER_ADDR_FMT_SIZE];
	char dmac[RTE_ETHER_ADDR_FMT_SIZE];

	rte_ether_format_addr(smac, RTE_ETHER_ADDR_FMT_SIZE, &src_mac);
	rte_ether_format_addr(dmac, RTE_ETHER_ADDR_FMT_SIZE, &dst_mac);
    printf(" %x frame: [NULL | %s] => [NULL | %s]\n", ether_type, smac, dmac);
    return;
}

int main() {
    int client_sock;
    struct sockaddr_un server_addr;
    struct msghdr msgh;
    struct iovec iov;
    struct cmsghdr *cmsg;
    char control[CMSG_SPACE(32 * sizeof(int))];
    int fd_num;
    int fds[32];
    void *map_addr[32];
    int map_addr_num = 0;
    struct walk_arg wa;
    struct stat sb;

    // 创建 AF_UNIX 套接字
    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 连接到服务器
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    // 初始化msghdr结构
    memset(&msgh, 0, sizeof(msgh));
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_control = control;
    msgh.msg_controllen = sizeof(control);

    // 初始化iov结构
    iov.iov_base = &wa;
    iov.iov_len = sizeof(wa);

    // 接收消息
    int ret = recvmsg(client_sock, &msgh, MSG_CMSG_CLOEXEC);
    if(ret <= 0) {
        if(ret)
            printf("recvmsg failed\n");
        else
            printf("recvive no packets\n");
    }

    for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL;
		cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
		if ((cmsg->cmsg_level == SOL_SOCKET) &&
			(cmsg->cmsg_type == SCM_RIGHTS)) {
			fd_num = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
			memcpy(fds, CMSG_DATA(cmsg), fd_num * sizeof(int));
			break;
		}
	}

    for(int i = 0; i < fd_num; i++) {
        if(fstat(fds[i], &sb) == 0) {
            // 打印文件的状态信息
            printf("File information: %d\n", fds[i]);
            printf("    Device ID: %jd\n", (intmax_t)sb.st_dev);
            printf("    Inode number: %jd\n", (intmax_t)sb.st_ino);
            printf("    File type and mode: %o\n", sb.st_mode);
            printf("    Number of hard links: %jd\n", (intmax_t)sb.st_nlink);
            printf("    User ID of owner: %jd\n", (intmax_t)sb.st_uid);
            printf("    Group ID of owner: %jd\n", (intmax_t)sb.st_gid);
            printf("    Device ID of file: %jd\n", (intmax_t)sb.st_rdev);
            printf("    Size of file, in bytes: %jd\n", (intmax_t)sb.st_size);
            printf("    Time of last access: %s", ctime(&sb.st_atime));
            printf("    Time of last modification: %s", ctime(&sb.st_mtime));
            printf("    Time of last status change: %s\n", ctime(&sb.st_ctime));
        }
        // close(fds[i]);
        map_addr[i] = mmap(NULL, sb.st_size, (PROT_READ|PROT_WRITE), MAP_SHARED, fds[i], 0);
        map_addr_num++;
        printf("map_addr[%d] = %p\n", i, map_addr[i]);
        printf("region_nr: %d\n", wa.region_nr);
        printf("wa_guest_phys_addr: %lu\n", wa.regions[i].guest_phys_addr);
        printf("wa_userspace_addr: %lu\n", wa.regions[i].userspace_addr);
        printf("wa_mmap_offset: %lu\n", wa.regions[i].mmap_offset);
        printf("wa_memory_size: %lu\n", wa.regions[i].memory_size);
    }
    
    struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	struct rte_ether_addr src_mac, dst_mac;
	uint16_t ether_type;
    struct rte_mbuf *pkt = (struct rte_mbuf *)((uint64_t)wa.data - wa.regions[0].userspace_addr + (uint64_t)map_addr[0]);
    pkt->buf_addr = (void*)((uint64_t)pkt->buf_addr - wa.regions[0].userspace_addr + (uint64_t)map_addr[0]);
    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
    src_mac = eth_hdr->src_addr;
    dst_mac = eth_hdr->dst_addr;
    ether_type = eth_hdr->ether_type;
    show_packet(src_mac, dst_mac, ether_type);

    for(int i = 0; i < fd_num; i++) {
        close(fds[i]);
    }

    // 关闭套接字
    close(client_sock);

    return 0;
}