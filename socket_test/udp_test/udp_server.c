#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[] ){

    //服务器端口号
    unsigned short port = 8000;    
    if( argc > 1){
        port = atoi(argv[1]);    
    }
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in my_addr;
    // bzero(&my_addr,sizeof(my_addr));
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htons(INADDR_ANY);
    printf("Binding server to port %d \n", port);

    int err_log = 0;
    err_log = bind(sockfd, (struct sockaddr*)& my_addr, sizeof(my_addr));
    if(err_log != 0){
        perror("bind");
        close(sockfd);
        exit(-1);
    }
	// printf("seceive data !\n");

	while(1){
	    int recv_len;
        char recv_buf[512] = "";
        char cli_ip[INET_ADDRSTRLEN]="";  // INET_ADDRSTRLEN = 16
        struct sockaddr_in client_addr;
        socklen_t cliaddr_len = sizeof(client_addr);

        /*接收数据*/
        recv_len=recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&client_addr, &cliaddr_len);
        inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);
        printf("ip:%s, port:%d\n", cli_ip, ntohs(client_addr.sin_port));
        printf("data(%d):%s\n", recv_len, recv_buf);

        /*发送数据*/
        char send_buf[512] = "";
        printf("server send:");
        fgets(send_buf,sizeof(send_buf),stdin);   //获取输入
        send_buf[strlen(send_buf)-1] = '\0';
        sendto(sockfd, send_buf,strlen(send_buf), 0,(struct sockaddr*)&client_addr, sizeof(client_addr));
	}
    close(sockfd);
    return 0;
}
