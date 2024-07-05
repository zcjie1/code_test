#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ADD_COUNT 100000

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

#ifdef MUTEX
    pthread_mutex_init(&lock, NULL);
#elif defined SPINLOCK
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
#endif

    // 创建线程
    pthread_create(&thread1, NULL, inc_func, NULL);
    pthread_create(&thread2, NULL, inc_func, NULL);

    // 等待线程结束
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Final value of shared_var is %d\n", shared_var);

#ifdef MUTEX
    pthread_mutex_destroy(&lock);
#elif defined SPINLOCK
    pthread_spin_destroy(&spinlock);
#endif

    return 0;
}