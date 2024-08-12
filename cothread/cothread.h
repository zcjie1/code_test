#ifndef __COTHREAD_H__
#define __COTHREAD_H__

#define STACK_SIZE 8192

typedef void* (*FunPointer)(void*, void*); //函数指针

typedef struct {
    void* r15;
    void* r14;
    void* r13;
    void* r12;
    void* r9;
    void* r8;

    void* rbp;
    void* rdi;
    void* rsi;
    void* rip;

    void* rdx;
    void* rcx;
    void* rbx;
    void* rsp;
}Regs;

// 协程上下文
typedef struct Cothread {
    Regs reg;
    char *stack_base;
    char *sp;
}Cothread;

Cothread* cothread_create(Cothread* ctx, FunPointer fn, const void* para1, const void* para2);

extern void cothread_swap(Cothread* prev, Cothread* next);

#endif // !__COTHREAD_H__