#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc , char *argv[]){

	//服务器端口号，也可以设置IP地址用于连接
	unsigned short port = 8000;

	/* main 函数传参*/
	if(argc > 1)	//函数参数，可以更改服务器端口号
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

    /* 组织本地信息*/
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* 绑定信息 */
	int err_log = bind(sockfd, (struct sockaddr*)&my_addr,			  	    sizeof(my_addr));
    if(err_log != 0) {
        perror("bind");
        close(sockfd);
        exit(-1);
    }

    err_log = listen(sockfd,10);
    if(err_log != 0){
        perror("listen");
        close(sockfd);
        exit(-1);
    }

    printf("listen client @port=%d \r\n",port);
    struct sockaddr_in client_addr;
    char cli_ip[INET_ADDRSTRLEN] = "";
    socklen_t cliaddr_len = sizeof(client_addr);

    /* 等待接收可连接的文件描述符 */
    int connfd;
    connfd = accept(sockfd, (struct sockaddr*)&client_addr, &cliaddr_len);
    if(connfd < 0) {
        perror("accept");
        exit(-1);
    }	
    inet_ntop(AF_INET,&client_addr.sin_addr, cli_ip,INET_ADDRSTRLEN);
    printf("client ip=%s,port=%d\r\n", cli_ip,ntohs(client_addr.sin_port));

    while(1){
	    char recv_buf[2048] = "";
	    if(recv(connfd, recv_buf, sizeof(recv_buf),0)> 0){
	        printf("recv data:\r\n");
	        printf("%s\r\n",recv_buf);
	        char send_buf[512] = "";
	        /* 发送消息 */
	        printf("send:");
	        fgets(send_buf, sizeof(send_buf),stdin);
	        send_buf[strlen(send_buf)-1]=0;
	        send(connfd, send_buf, strlen(send_buf),0);
	    }else{
            break;
        }
    }
    close(connfd);
    printf("client closed!\r\n");
    close(sockfd);
    return 0;
}


