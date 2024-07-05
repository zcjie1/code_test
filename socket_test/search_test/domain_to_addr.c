#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
 
int main(int argc, char **argv) {
     char   **pptr;
     struct hostent *hptr;
     char   str[16];
     char ptr[64]="www.baidu.com";

     hptr = gethostbyname(ptr);
     if(hptr == NULL) {
        printf(" gethostbyname error for host:%s\n", ptr);
        return 0;
     }

    printf("official hostname:%s\n",hptr->h_name);
    for(pptr = hptr->h_aliases; *pptr != NULL; pptr++)
        printf(" alias:%s\n",*pptr);
    switch(hptr->h_addrtype) {
        case AF_INET:
        case AF_INET6:
            pptr=hptr->h_addr_list;
            for(; *pptr!=NULL; pptr++) {
                inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
                printf(" address:%s\n",str);
            }
            inet_ntop(hptr->h_addrtype, *(hptr->h_addr_list), str, sizeof(str));
            printf(" first address: %s\n",str);
            break;
        default:
            printf("unknown address type\n");
            break;
    }
    return 0; 
}
