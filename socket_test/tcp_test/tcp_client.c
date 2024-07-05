#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

// void sigint_handler(int signo, int sockfd) {
//     close(sockfd);
//     printf("Socket closed. Exiting...\n");
//     exit(0);
// }


int main(int argc , char *argv[]) {
    unsigned short port = 8000;  //服务器端口号，不设置服务器固定IP地址

    /* main 函数传参*/
    if(argc > 1)   //函数参数，可以更改服务器端口号
    {
        port = atoi(argv[1]);
    }

    /* 创建TCP套接字*/
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket");
        exit(-1);
    }

    /* 设置要连接的IP地址和端口 */
    // char ip_addr[] = "127.0.0.1";
    // unsigned int ip = 0;
    // inet_pton(AF_INET, ip_addr, &ip);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    // bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* 连接服务器*/
    int err_log = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(err_log != 0) {
        perror("connect");
        close(sockfd);
        exit(-1);
    }

    // /* 设置SIGINT(Crtl+C)信号的处理函数 */
    // signal(SIGINT, sigint_handler);

    while(1) {
        char send_buf[512] = "";
        char recv_buf[512] = "";

        /* 发送消息 发送#退出 */
        printf("send:");
        fgets(send_buf, sizeof(send_buf),stdin);
        send_buf[strlen(send_buf)-1]=0;
        if(send_buf[0] =='#')
            break;
        send(sockfd, send_buf, strlen(send_buf),0);

        /* 接收消息 */
        int i;
        i=recv(sockfd, recv_buf, sizeof(recv_buf),0);
        printf("i=%d\r\n",i);
        printf("recv:%s\r\n",recv_buf);
    }
    close(sockfd);
    printf("client exiting...\r\n");
    return 0;
}
