/** https://blog.csdn.net/wenwuge_topsec/article/details/9628409
*  This is a simple demon of uio driver.  
*  Last modified by   
        09-05-2011   Joseph Yang(Yang Honggang)<ganggexiongqi@gmail.com>  
*  
* Compile:    
*   Save this file name it simple.c  
*   # echo "obj-m := simple.o" > Makefile  
*   # make -Wall -C /lib/modules/`uname -r`/build M=`pwd` modules  
* Load the module:  
*   #modprobe uio  
*   #insmod simple.ko  
*/  
  
#include <linux/module.h>  
#include <linux/platform_device.h>  
#include <linux/uio_driver.h>  
#include <linux/slab.h> /* kmalloc, kfree */

#define MEM_SIZE 1024

struct uio_info kpart_info = {
	.name = "kpart",  
	.version = "0.1",  
	.irq = UIO_IRQ_NONE, // 不产生中断
	.mem[0] = {
        .addr = (phys_addr_t)NULL,
        .size = MEM_SIZE,
        .memtype = UIO_MEM_LOGICAL, // 逻辑地址(虚拟内存)
    },
};
  
static int drv_kpart_probe(struct platform_device *pdev);
static int drv_kpart_remove(struct platform_device *pdev);

// uio设备probe函数
static int drv_kpart_probe(struct platform_device *pdev)  
{
	int ret;

	printk("drv_kpart_probe( %p )\n", pdev);

	kpart_info.mem[0].internal_addr = kmalloc(MEM_SIZE, GFP_KERNEL); // mmap区域
	if (!kpart_info.mem[0].internal_addr) {
        dev_err(&pdev->dev, "Failed to allocate memory\n");
        return -ENOMEM;
    }

	// 设置 addr 字段为分配的虚拟地址
    kpart_info.mem[0].addr = (phys_addr_t)(uintptr_t)kpart_info.mem[0].internal_addr;

	// 注册uio设备
	ret = uio_register_device(&pdev->dev, &kpart_info);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register UIO device\n");
        kfree(kpart_info.mem[0].internal_addr);
        return ret;
    }
 
	return 0;  
}  

// uio设备remove函数
static int drv_kpart_remove(struct platform_device *pdev) 
{
	uio_unregister_device(&kpart_info);
    kfree(kpart_info.mem[0].internal_addr);
    dev_info(&pdev->dev, "UIO device unregistered and memory freed\n");
    return 0; 
}

// platform设备在无实际硬件设备的条件下，可以 "自动探测", 调用probe函数
static struct platform_driver uio_dummy_driver = {
    .probe = drv_kpart_probe,
    .remove = drv_kpart_remove,
    .driver = {
        .name = "kpart",
        .owner = THIS_MODULE,
    },
};

static int __init uio_kpart_init(void)  
{       
	return platform_driver_register(&uio_dummy_driver);
}  
  
static void __exit uio_kpart_exit(void)  
{  
    platform_driver_unregister(&uio_dummy_driver);
}  
  
module_init(uio_kpart_init);  
module_exit(uio_kpart_exit);  
  
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("Benedikt Spranger");  
MODULE_DESCRIPTION("UIO dummy driver");