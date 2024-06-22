#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/unix_socket_example"
#define BUFFER_SIZE 128

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];

    // 创建 Unix Domain Socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // 确保旧的套接字文件不存在
    unlink(SOCKET_PATH);

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 绑定套接字到路径
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, 5) == -1) {
        perror("listen error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("服务器正在等待连接...\n");

    // 接受连接
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
        perror("accept error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 接收数据
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(client_fd, buffer, BUFFER_SIZE, 0) == -1) {
        perror("recv error");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("收到数据: %s\n", buffer);

    // 发送数据
    const char *response = "Hello from server!";
    if (send(client_fd, response, strlen(response), 0) == -1) {
        perror("send error");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 关闭连接
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}
