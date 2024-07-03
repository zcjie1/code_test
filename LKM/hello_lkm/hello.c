#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZCJ");
MODULE_DESCRIPTION("A simple Hello World module");
MODULE_VERSION("1.0");

static int __init hello_init(void) {
    pr_info("Hello, World!\n");
    return 0;
}

static void __exit hello_exit(void) {
    pr_info("Goodbye, World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
