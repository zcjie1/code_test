#define _GNU_SOURCE
#include <stdio.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <unistd.h>
#include <sys/mman.h>  
#include <errno.h>
#include <string.h>
  
#define UIO_DEV "/dev/uio0"  
#define UIO_ADDR "/sys/class/uio/uio0/maps/map0/addr"  
#define UIO_SIZE "/sys/class/uio/uio0/maps/map0/size"  
  
static char uio_addr_buf[18], uio_size_buf[18];
  
int main(void)
{  
    int uio_fd, addr_fd, size_fd;
    int uio_size;
    void* uio_addr, *access_address, *new_access_address;

    char str[] = "Hello, UIO Device!";
    
    // 打开相关文件
    uio_fd = open(UIO_DEV, O_RDWR);
    addr_fd = open(UIO_ADDR, O_RDONLY);
    size_fd = open(UIO_SIZE, O_RDONLY);
    if( addr_fd < 0 || size_fd < 0 || uio_fd < 0) {
        // close fd 
        fprintf(stderr, "open error: %s\n", strerror(errno));
        exit(-1);  
    }

    // 读取uio设备内存起始地址和大小
    read(addr_fd, uio_addr_buf, sizeof(uio_addr_buf));
    read(size_fd, uio_size_buf, sizeof(uio_size_buf)); 
    uio_addr = (void*)strtoul(uio_addr_buf, NULL, 0);
    uio_size = (int)strtoq(uio_size_buf, NULL, 16);
    
    // 映射uio设备文件
    access_address = mmap(NULL, uio_size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, 0);  
    if ( access_address == (void*) -1) {
        // close fd
        fprintf(stderr, "mmap error: %s\n", strerror(errno));
        exit(-1);  
    }  
    printf("The device address: %p (length=%d)\n"
            "logical address: %p\n", 
            uio_addr, uio_size, access_address);

    // 读写操作，使得mmap虚拟内存分配对应物理内存
    memcpy(access_address, str, strlen(str) + 1);
    fprintf(stderr, "%s\n", (char*)access_address);

    // mremap测试(需要分配物理内存)
    new_access_address = mremap(access_address, getpagesize(),
                        getpagesize(),
                        MREMAP_MAYMOVE); 
    if (new_access_address == (void*) -1) {
        fprintf(stderr, "mremmap error: %s\n", strerror(errno));
        munmap(access_address, uio_size + getpagesize() + 1111);
        close(uio_fd);
        close(addr_fd);
        close(size_fd);
        exit(-1); 
    }   
    printf(">>>AFTER REMAP: logical address %p\n", new_access_address);
    
    // 清理资源，退出程序
    munmap(new_access_address, uio_size + getpagesize() + 1111);
    close(uio_fd);
    close(addr_fd);
    close(size_fd);
  
    return 0;  
}