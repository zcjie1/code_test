#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<unistd.h>

int main(int argc,char *argv[]){
    int fd,len;
    char *ptr;

    if(argc < 2){
        printf("please enter a file\n");    
        return 0;
    }

    if((fd = open(argv[1], O_RDWR)) < 0){
        perror("open file error");
        return -1; 
    }

    len = lseek(fd, 0, SEEK_END);

    // prot must be the same as o_flags   
    ptr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr==MAP_FAILED){
        perror("mmap error");
        close(fd);
        return -1; 
    }

    close(fd);

    printf("length is %lu\n", strlen(ptr));
    printf("the %s content is: %s\n", argv[1], ptr);
    ptr[0]='c';
    printf("the %s content is: %s\n",argv[1],ptr);

    munmap(ptr,len);

    return 0;
}
