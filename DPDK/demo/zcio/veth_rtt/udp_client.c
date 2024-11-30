#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define SERVER_IP "172.17.0.3"   // 服务器IP地址
#define PORT 8888               // 服务器端口
#define SEND_COUNT 60           // 发送数据包的次数
#define DATA_SIZE 64            // 缓冲区中有效数据的大小

// 函数：生成字母并填充到缓冲区
void fill_letters(char *buffer, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 'A';
    }
    buffer[size] = '\0';  // 确保字符串以null结尾
}

// UDP数据包结构体
struct udp_packet{
    struct timespec timestamp;  // 时间戳
    char payload[DATA_SIZE];   // 有效负载
}__attribute__((packed));

int main() {
    uint64_t count = 0;
    int sockfd;
    struct sockaddr_in server_addr;
    struct udp_packet packet;
    struct timespec send_time, recv_time;
    uint64_t rtt_sec[SEND_COUNT], rtt_nsec[SEND_COUNT];

    // 创建UDP套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    printf("UDP client started\n");

    while (count < SEND_COUNT) {
        // 填充字母作为有效负载
        fill_letters(packet.payload, DATA_SIZE);

        // 获取当前时间作为时间戳
        clock_gettime(CLOCK_MONOTONIC_RAW, &send_time);
        memcpy(&packet.timestamp, &send_time, sizeof(struct timespec));

        // 发送数据包到服务器
        sendto(sockfd, &packet, sizeof(struct udp_packet), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // 接收回显的数据包
        int n = recvfrom(sockfd, &packet, sizeof(struct udp_packet), 0, NULL, NULL);
        if (n > 0 && n == sizeof(struct udp_packet)) {
            // 记录接收时间
            clock_gettime(CLOCK_MONOTONIC_RAW, &recv_time);

            // 计算RTT（往返时间）
            rtt_sec[count] = (recv_time.tv_sec - packet.timestamp.tv_sec);  // 记录rtt秒
            rtt_nsec[count] = (recv_time.tv_nsec - packet.timestamp.tv_nsec);  // 记录rtt纳秒
            count++;
            printf("recv %lu rtt data\n", count);
        }else if(n > 0) {
            printf("recv a useless packet\n");
        }

        usleep(100000);  // 每隔0.1秒发送一次
    }

    uint64_t total_sec = 0, total_nsec = 0;
    for(uint64_t i = 0; i < count; i++) {
        total_sec += rtt_sec[i];
        total_nsec += rtt_nsec[i];
    }

    uint64_t rtt = (total_sec * 1000000000ULL + total_nsec) / count / 1000;
    printf("Average RTT for %lu packets: %lu us\n", count, rtt);

    close(sockfd);
    return 0;
}