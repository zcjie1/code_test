#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main()
{
    int fd;
    char path[32] = "/dev/virtual_cdev-0";
    char buf0[32] = "hello";
    char buf1[32] = "word";
    char buf2[32] = {0};
    int ret = 0;

    fd = open(path, O_RDWR);
    if (fd < 0){
        printf("open failed fd = %d\n", fd);
        return 0;
    }

    printf("opend\n");

    write(fd, buf0, strlen(buf0));
    write(fd, buf1, strlen(buf1));

    lseek(fd, 0, SEEK_SET);
    ret = read(fd, buf2, strlen(buf0) + strlen(buf1));
    printf("%s\n", buf2);

    close(fd);
    printf("closed\n");
    return 0;
}