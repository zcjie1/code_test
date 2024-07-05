#include <stdio.h>

int main(){
    
    int a = 0;
    int b = 1;
    __asm__(
        "movl %1, %0\n\t"
        :"=m" (a)
        :"r" (b)
    );

    printf("%d\n", a);
    return 0;
}
