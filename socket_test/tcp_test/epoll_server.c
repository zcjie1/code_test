#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define MAXLINE 20
#define LISTENQ 20
int main(int argc, char* argv[]){
    int i,maxi,listenfd,connfd,sockfd,epfd,nfds,portnumber=8000;
    ssize_t n;
    char line[MAXLINE];
    socklen_t clilen;
    char buf[512];
    if( argc >= 2 ){
        if( (portnumber = atoi(argv[1])) < 0 )
        {
            fprintf(stderr,"Usage: %s innvalid portnumber\r\n",argv[0]);
            return 1;
        }
    }

    //声明epoll_event结构体的变量，ev用于注册事件, 
    //events数组用于回传要处理的事件
    struct epoll_event ev,events[20];

    //生成用于处理accept的epoll专用的文件描述符
    epfd = epoll_create(256);

    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        perror("listenfd");
        exit(-1);
    }

    //设置与要处理的事件相关的文件描述符
    ev.data.fd = listenfd;
    //设置要处理的事件类型
    ev.events = EPOLLIN|EPOLLET;
    //注册epoll事件
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

    /* 组织本地信息*/
    // bzero(&serveraddr, sizeof(serveraddr));
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portnumber);
    serveraddr.sin_addr.s_addr = htons(INADDR_ANY);
    bind(listenfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, LISTENQ);
    maxi = 0;
    while(1) {
	    //等待epoll事件的发生
	    nfds = epoll_wait(epfd,events,20,500);

	    //处理所发生的所有事件
	    for(i=0; i<nfds; i++){
		    /*如果新监测到一个socket用户连接到了绑定的socket端口，则建立新的连接*/
	        if(events[i].data.fd==listenfd){
                clilen = sizeof(clientaddr);
                connfd = accept(listenfd,(struct sockaddr*)&clientaddr, &clilen);
                if(connfd<0) {
                    perror("connfd<0");
                    exit(1);
                }
                char *str = inet_ntoa(clientaddr.sin_addr);
                printf("accapt a connection from %s\r\n", str);

                //设置用于读操作的文件描述符
                ev.data.fd=connfd;
                //设置用于注册的读操作事件
                ev.events=EPOLLIN|EPOLLET;

                //ev.events=EPOLLIN;
                //注册ev
                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
            }
            else if(events[i].events&EPOLLIN){
                //如果是已经连接的用户，并收到了数据，则进行读入
                printf("EPOLLIN: \r\n");
	            if( (sockfd = events[i].data.fd) < 0)
                    continue;
	            if( (n = read(sockfd, line, MAXLINE)) < 0){
                    if(errno == ECONNRESET){
                        close(sockfd);
                        events[i].data.fd = -1;
                        continue;
                    }else{
                        printf("readline error\r\n");
                        continue;
                    }
                }else if(n == 0){
                    close(sockfd);
                    events[i].data.fd = -1;
                    continue;
                }
	            line[n-1] = '\0';
	            printf("recv: ");
		        printf("%s\r\n",line);
		        ev.data.fd=sockfd; 	        
		        ev.events=EPOLLOUT|EPOLLET; 
	            //修改sockfd上要处理的事件为EPOLLOUT
	            epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);

	        }
            else if(events[i].events&EPOLLOUT){ // 如果有数据发送
	            sockfd = events[i].data.fd;
	            printf("send: ");

                // 获取当前时间
                time_t current_time;
                time(&current_time);
            
                // 将时间转换为字符串格式
                char *time_string = ctime(&current_time);
                time_string[strlen(time_string) - 1] = '\0'; // 移除末尾的换行符

                printf("%s\n",time_string);

	            //fgets(buf, sizeof(buf),stdin);
	            //buf[strlen(buf)-1]=0;
	            send(sockfd, time_string, strlen(time_string), 0);
		        ev.data.fd=sockfd; //设置用于读操作的文件描述符
		        ev.events=EPOLLIN|EPOLLET; //设置用于注册的读操作事件
	            epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev); //修改sockfd上要处理的事件为EPOLLIN
	        }
        }
    }
    return 0;
} 
