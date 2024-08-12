# 指定编译器
CC = gcc

# 指定编译器选项
CFLAGS = -Wall -g  # 启用警告和优化
ASFLAGS = -Wall    # 汇编器选项

# 定义源文件
C_SOURCES = main.c cothread.c
ASM_SOURCES = swap.s

# 定义目标文件（.o文件）
OBJECTS = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.s=.o)

# 可执行文件的名称
TARGET = my_program

# 默认目标：编译所有内容
all: $(TARGET)

# 链接生成最终的可执行文件
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^

# 生成每个C源文件的目标文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 生成每个汇编源文件的目标文件
%.o: %.s
	$(CC) $(ASFLAGS) -c $< -o $@

# 清除编译生成的文件
clean:
	rm -f $(OBJECTS) $(TARGET)
