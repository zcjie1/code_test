#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZCJ");
MODULE_DESCRIPTION("A simple Hello World module");
MODULE_VERSION("1.0");

// module_param 定义用户可传入参数
static int num = 666;
module_param(num, int, S_IRUGO);

static int row = 3;
static int colume = 4;
static int **matrix;

static int __init hello_init(void) {
    pr_info("Hello, World!\n");
    pr_info("num=%d\n", num);

    // 测试模块加载失败
    if(num < 5){
        pr_err("num is too small\n");
        return -EINVAL;
    }

    int size = 0;

    // 分配指针
    size = row * sizeof(int *);
    matrix = (int **)kzalloc(size, GFP_KERNEL);
    if(!matrix){
        pr_err("Out of Memorry!\n");
        return -ENOMEM;
    }

    // 分配矩阵
    size = colume * sizeof(int);
    for(int i=0; i<row; i++){
        matrix[i] = (int *)kzalloc(size, GFP_KERNEL);
        if(!matrix[i]){
            for(int j=0; j<i; j++)
                kfree(matrix[j]);
            kfree(matrix);
            pr_err("Out of Memorry!\n");
            return -ENOMEM;
        }
    }

    // 矩阵初始化
    for(int i=0; i<row; i++){
        for(int j=0; j<colume; j++){
             matrix[i][j] = get_random_u32_inclusive(0, 150);
        }
    }        

    pr_info("matrix is initialized!\n");

    // 输出矩阵
    for(int i=0; i<row; i++){
        pr_info("%5d %5d %5d %5d\n",
                matrix[i][0], matrix[i][1], 
                matrix[i][2], matrix[i][3]);
    }

    return 0;
}

static void __exit hello_exit(void) {
    if(!matrix) {
        for(int i=0; i<row; i++){
            if(!matrix[i])
                kfree(matrix[i]);
        }
        kfree(matrix);
    }
    pr_info("Goodbye, World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
