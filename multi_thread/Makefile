# raw 为非原子操作
# mutex 为互斥锁实现原子操作
# spinlock 为自旋锁实现原子操作
# asm 为内联汇编实现原子操作

CC := gcc
CFLAGS := -Wall -g

SRC := multi_thread.c
TARGET := main

EXCUTE := ./main

default: raw mutex spinlock asm

raw: $(SRC)
	@$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
	@printf "%-15s" "RAW MODE: "
	@$(EXCUTE)

mutex: $(SRC)
	@$(CC) $(CFLAGS) -D MUTEX $(SRC) -o $(TARGET)
	@printf "%-15s" "MUTEX MODE: "
	@$(EXCUTE)

spinlock: $(SRC)
	@$(CC) $(CFLAGS) -D SPINLOCK $(SRC) -o $(TARGET)
	@printf "%-15s" "SPINLOCK MODE: "
	@$(EXCUTE)

asm: $(SRC)
	@$(CC) $(CFLAGS) -D ASM $(SRC) -o $(TARGET)
	@printf "%-15s" "ASM MODE: "
	@$(EXCUTE)


.PHONY: clean

clean:
	@rm ./main
