#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zcj");
MODULE_DESCRIPTION("Hello Char Device");
MODULE_VERSION("1.0");
MODULE_ALIAS("hello_cdev");

static dev_t hello_device_num = 0;
static struct cdev *hello_cdev_ptr = NULL;
static struct class *hello_cdev_class = NULL;
static struct device *hello_cdev_device = NULL;

int hello_device_open(struct inode *pinode, struct file *pfile)
{
    pr_info("hello_device opened: major is %d, minor is %d\n", imajor(pinode), iminor(pinode));
    return 0;
}

int hello_device_release(struct inode *pinode, struct file *pfile)
{
    pr_info("hello_device closed: major is %d, minor is %d\n", imajor(pinode), iminor(pinode));
    return 0;
}

// hello char device的设备文件操作函数
static const struct file_operations hello_device_ops = {
    .owner = THIS_MODULE,
    .open = hello_device_open,
    .release = hello_device_release,
    .read = NULL,
    .write = NULL
};

static int __init hello_device_init(void)
{
    int ret = 0;

    // 分配设备号
    ret = alloc_chrdev_region(&hello_device_num, 0, 2, "hello_device");
    if(ret){
        pr_err("Failed: alloc_chrdev_region for hello_device!\n");
        goto failed_alloc_chrdev_region;
    }

    // 分配cdev
    hello_cdev_ptr = cdev_alloc();
    if(!hello_cdev_ptr){
        pr_err("Failed : cdev_alloc for hello_cdev\n");
        goto failed_cdev_alloc;
    }

    // cdev初始化
    hello_cdev_ptr->owner = THIS_MODULE;
    hello_cdev_ptr->ops = &hello_device_ops;

    // 注册cdev类
    ret = cdev_add(hello_cdev_ptr, hello_device_num, 2);
    if(ret){
        pr_err("Failed: cdev_add for hello_cdev\n");
        goto failed_cdev_add;
    }

    // 注册device class
    hello_cdev_class = class_create("hello_device_class");
    if(IS_ERR(hello_cdev_class)){
        pr_err("Failed: calss_create for hello_device!\n");
        goto failed_cdev_add;
    }

    // 注册device实例
    hello_cdev_device = device_create(hello_cdev_class, NULL, hello_device_num, NULL, "hello_device");
    if(!hello_cdev_device){
        pr_err("Failed: device_create for hello_device");
        goto failed_device_create;
    }

    pr_info("device initialized: major is %u, minor is %u\n", MAJOR(hello_device_num), MINOR(hello_device_num));
    return 0;

failed_device_create:
    class_destroy(hello_cdev_class);

failed_cdev_add:
    cdev_del(hello_cdev_ptr);

failed_cdev_alloc:
    unregister_chrdev_region(hello_device_num, 1);

failed_alloc_chrdev_region:
    return ret;
}

static void __exit hello_device_exit(void)
{
    if(hello_cdev_device)
        device_destroy(hello_cdev_class, hello_device_num);

    if(hello_cdev_class)
        class_destroy(hello_cdev_class);

    if(hello_cdev_ptr)
        cdev_del(hello_cdev_ptr);

    if(hello_device_num)
        unregister_chrdev_region(hello_device_num, 2);

    pr_info("device exited!\n");
}

module_init(hello_device_init);
module_exit(hello_device_exit);