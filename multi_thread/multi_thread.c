#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define ADD_COUNT 10000000

int shared_var = 0;

#ifdef MUTEX
pthread_mutex_t lock; // 互斥锁
#elif defined SPINLOCK
pthread_spinlock_t spinlock; // 自旋锁
#endif

void* inc_func(void *arg) {
    int i;
    for(i = 0; i < ADD_COUNT; i++) {
#ifdef MUTEX
        pthread_mutex_lock(&lock);
#elif defined SPINLOCK
        pthread_spin_lock(&spinlock);
#endif

#ifndef ASM
        shared_var++;
#endif

#ifdef MUTEX
        pthread_mutex_unlock(&lock);
#elif defined SPINLOCK
        pthread_spin_unlock(&spinlock);
#endif

#ifdef ASM
        asm volatile(
            "lock incl %0\n\t"
            : "+m" (shared_var)
            :: "memory"
        );
#endif

    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    struct timespec start, end;

#ifdef MUTEX
    pthread_mutex_init(&lock, NULL);
#elif defined SPINLOCK
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
#endif
    // 记录程序开始时间
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 创建线程
    pthread_create(&thread1, NULL, inc_func, NULL);
    pthread_create(&thread2, NULL, inc_func, NULL);

    // 等待线程结束
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // 记录程序结束时间
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long start_ns = start.tv_sec * 1000000000 + start.tv_nsec;
    long long end_ns = end.tv_sec * 1000000000 + end.tv_nsec;
    long long elapsed_time = end_ns - start_ns;

    printf("   %d          %lldns\n", shared_var, elapsed_time);

#ifdef MUTEX
    pthread_mutex_destroy(&lock);
#elif defined SPINLOCK
    pthread_spin_destroy(&spinlock);
#endif

    return 0;
}