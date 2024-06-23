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

struct uio_info kpart_info = {
	.name = "kpart",  
	.version = "0.1",  
	.irq = UIO_IRQ_NONE, // 不产生中断
	// 若无实际硬件设备但需要中断，可以设置为UIO_IRQ_CUSTOM
};
  
static int drv_kpart_probe(struct device *dev);  
static int drv_kpart_remove(struct device *dev);

static struct device_driver uio_dummy_driver = {  
	.name = "kpart",  
	.bus = &platform_bus_type,  
	.probe = drv_kpart_probe,  
	.remove = drv_kpart_remove,  
};  
  
static int drv_kpart_probe(struct device *dev)  
{  
	printk("drv_kpart_probe( %p )\n", dev);
	kpart_info.mem[0].addr = (unsigned long)kmalloc(1024,GFP_KERNEL); // mmap区域

	if(kpart_info.mem[0].addr == 0)  
		return -ENOMEM;  
	kpart_info.mem[0].memtype = UIO_MEM_LOGICAL; // 逻辑地址
	kpart_info.mem[0].size = 1024; // mmap区域size

	// 注册uio设备
	if(uio_register_device(dev, &kpart_info))  
		return -ENODEV;  
	return 0;  
}  
  
static int drv_kpart_remove(struct device *dev)  
{  
    uio_unregister_device(&kpart_info);  
  
    return 0;  
}  

// platform设备在无实际硬件设备的条件下，可以 "自动探测"
// 调用probe函数
static struct platform_device *uio_dummy_device;  
  
static int __init uio_kpart_init(void)  
{       
    // 创建platform设备
    uio_dummy_device = platform_device_register_simple("kpart", -1, NULL, 0);
	
	// 注册device_driver类型的uio_dummy_driver变量到bus
    return driver_register(&uio_dummy_driver); 
}  
  
static void __exit uio_kpart_exit(void)  
{  
    platform_device_unregister(uio_dummy_device);  
    driver_unregister(&uio_dummy_driver);
}  
  
module_init(uio_kpart_init);  
module_exit(uio_kpart_exit);  
  
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("Benedikt Spranger");  
MODULE_DESCRIPTION("UIO dummy driver");