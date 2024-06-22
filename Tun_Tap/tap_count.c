#define _DEFAULT_SOURCE
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <sys/types.h>
#include <linux/if_tun.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 创建TAP_DEV设备
 * 循环从TUN_DEV设备读取数据
 * 输出读取字节数
*/

#define TAP_DEV "test_tap"
#define BUFFER_SIZE 1024

int tap_alloc(char *name, int flags)
{

    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";

    if ((fd = open(clonedev, O_RDWR)) < 0) {
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        perror("ioctl error");
        close(fd);
        return err;
    }

    // 设置IP地址，并启动TAP设备
    system("sudo ip addr add 192.168.2.1/24 dev test_tap");
    // system("sudo ip link set dev test_tap master br1");
    // system("sudo ip link set dev br1 up");
    system("sudo ip link set dev test_tap up");

    printf("Open tun/tap device: %s for reading...\n", ifr.ifr_name);

    return fd;
}

int main()
{

    int tun_fd, nread;
    char buffer[BUFFER_SIZE];

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *        IFF_NO_PI - Do not provide packet information
     */
    tun_fd = tap_alloc(TAP_DEV, IFF_TAP | IFF_NO_PI);

    if (tun_fd < 0) {
        perror("Allocating interface");
        exit(1);
    }

    while (1) {
        nread = read(tun_fd, buffer, BUFFER_SIZE);
        if (nread < 0) {
            perror("Reading from interface");
            close(tun_fd);
            exit(1);
        }

        printf("Read %d bytes from tun/tap device\n", nread);
    }
    return 0;
}
