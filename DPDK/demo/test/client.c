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
};

int main() {
    int client_sock;
    struct sockaddr_un server_addr;
    struct msghdr msgh;
    struct iovec iov;
    struct cmsghdr *cmsg;
    char control[CMSG_SPACE(32 * sizeof(int))];
    int fd_num;
    int fds[32];
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
    iov.iov_len = 1;

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
        close(fds[i]);
    }

    // 关闭套接字
    close(client_sock);

    return 0;
}