#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "172.17.0.3"   // 服务器IP地址
#define PORT 8888              // 服务器端口
#define DATA_SIZE 64           // 缓冲区中有效数据的大小

// UDP数据包结构体
struct udp_packet{
    struct timespec timestamp;  // 时间戳
    char payload[DATA_SIZE];   // 有效负载
}__attribute__((packed));

int main() {
    uint64_t count = 0;
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    struct udp_packet packet;
    socklen_t addr_len = sizeof(client_addr);

    // 创建UDP套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(PORT);

    // 绑定套接字到指定端口
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP Server is listening on port %d\n", PORT);

    while (1) {
        // 接收来自客户端的数据包
        int n = recvfrom(sockfd, &packet, sizeof(struct udp_packet), 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n > 0 && n == sizeof(struct udp_packet)) {
            count++;
            printf("recv %lu packet\n", count);

            // 回显收到的数据包
            sendto(sockfd, &packet, sizeof(struct udp_packet), 0, (struct sockaddr*)&client_addr, addr_len);
        }
    }

    close(sockfd);
    return 0;
}