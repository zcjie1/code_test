#include <stdio.h>
#include <arpa/inet.h>
int main(int argc, char *argv[])
{
	    char ip_str[16]={0};
	    unsigned char ip[] = {172,20,223,75};
	    inet_ntop(AF_INET,( unsigned int *)ip,ip_str,16);
        printf("len of unsigned int: %ld\n", sizeof(unsigned int));
	    printf("ip_str = %s\n", ip_str);
	    return 0;
}
