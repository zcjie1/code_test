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
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];

    // 创建 Unix Domain Socket
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 连接到服务器
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect error");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 发送数据
    const char *message = "Hello from client!";
    if (send(client_fd, message, strlen(message), 0) == -1) {
        perror("send error");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 接收数据
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(client_fd, buffer, BUFFER_SIZE, 0) == -1) {
        perror("recv error");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("收到数据: %s\n", buffer);

    // 关闭连接
    close(client_fd);

    return 0;
}
