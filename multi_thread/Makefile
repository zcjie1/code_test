# raw 为非原子操作
# mutex 为互斥锁实现原子操作
# spinlock 为自旋锁实现原子操作
# asm 为内联汇编实现原子操作

CC := gcc
CFLAGS := -Wall -g

SRC := multi_thread.c
TARGET := main

EXCUTE := ./main

.PHONY: clean title

default: title raw mutex spinlock asm

title:
	@printf "MODE	Final-shared_var	  Cost Time\n"

raw: title $(SRC)
	@$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
	@printf "%-8s" "RAW "
	@$(EXCUTE)

mutex: title $(SRC)
	@$(CC) $(CFLAGS) -D MUTEX $(SRC) -o $(TARGET)
	@printf "%-8s" "MUTEX"
	@$(EXCUTE)

spinlock: title $(SRC)
	@$(CC) $(CFLAGS) -D SPINLOCK $(SRC) -o $(TARGET)
	@printf "%-8s" "SPIN"
	@$(EXCUTE)

asm: title $(SRC)
	@$(CC) $(CFLAGS) -D ASM $(SRC) -o $(TARGET)
	@printf "%-8s" "ASM"
	@$(EXCUTE)

clean:
	@rm ./main
