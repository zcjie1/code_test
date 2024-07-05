#include <stdio.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    printf("len of int: %ld\n", sizeof(int));
    int a = 0x0102030F;
    printf("len of short: %ld\n", sizeof(short));
    short int b = 0x0102;

    printf("htonl(0x%08x) = 0x%08x \n", a, htonl(a));
    printf("htonl(0x%08x) = 0x%08x \n", b, htonl(b));

    return 0;
}