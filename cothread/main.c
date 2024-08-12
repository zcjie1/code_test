#include <stdio.h>
#include <stdlib.h>
#include "cothread.h"

Cothread ctx1;
Cothread ctx2;

void* fun2() {
    printf("fun2\n");
    cothread_swap(&ctx2, &ctx1);
}

void* fun1() {
    cothread_create(&ctx1, fun1, 0, 0);
    cothread_create(&ctx2, fun2, 0, 0);
    printf("fun1\n");
    cothread_swap(&ctx1, &ctx2);
    char back[50] = "i am back\0";
    printf("%s", back); 
}

int main() {
    int a = 1;
    printf("abcd");
    fun1();
    return 0;
}