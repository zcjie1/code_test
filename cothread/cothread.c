#include "cothread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Cothread* cothread_create(Cothread* ctx,FunPointer fn, const void* para1, const void* para2) {
    ctx->stack_base = (char*)malloc(sizeof(char)*STACK_SIZE);
    char *sp = ctx->stack_base + STACK_SIZE - sizeof(void*);
    sp = (char*)((unsigned long)sp & -16LL); // 16字节对齐

    *sp = (void*)fn; 

    ctx->reg.rsp = sp;
    ctx->reg.rip = fn;

    ctx->reg.rdi = para1;
    ctx->reg.rsi = para2;

    return ctx;
}
