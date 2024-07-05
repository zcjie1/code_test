#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main( int argc, char *argv[] ){

    //服务器端口号
	unsigned short port = 8000;
	if(argc > 1){
        port = atoi(argv[1]);
    }

	int sockfd;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0) {
        perror("socket");
        exit(-1);
	}

    struct sockaddr_in dest_addr;
    // bzero(&dest_addr,sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    while(1) {
	    /* 发送数据 */
	    char send_buf[512] = "";
	    printf("client send:");
	    fgets(send_buf,sizeof(send_buf),stdin);
	    send_buf[strlen(send_buf)-1] = '\0';
	    sendto(sockfd, send_buf,strlen(send_buf), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

        int recv_len;
        char recv_buf[512] = "";
        char cli_ip[INET_ADDRSTRLEN]="";  // INET_ADDRSTRLEN = 16
        struct sockaddr_in client_addr;
        socklen_t cliaddr_len = sizeof(client_addr);

	    /*接收数据*/
	    recv_len=recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0 , (struct sockaddr*)&client_addr, &cliaddr_len);
	    inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);
	    printf("ip:%s, port:%d\n", cli_ip, ntohs(client_addr.sin_port));
	    printf("data(%d):%s\n", recv_len, recv_buf);
	}
	close(sockfd);
	return 0;
}
