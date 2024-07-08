#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>

#define VCD_NAME    "virtual_cdev"
#define VCD_COUNT   4

#define BUFFER_SIZE 1024

static dev_t FIRST_DEV_T = 0;
static unsigned int FIRST_MAJOR_DEV_T = 0;
static unsigned int FIRST_MINOR_DEV_T = 0;
static struct class *VCD_CLASS = NULL;
static struct cdev base_chrdev;
static bool is_cdev_added = false;

// 虚拟字符设备读写区域
struct buffer {
    char *data;
    int size;
};

// 虚拟字符设备
typedef struct virt_cdev {
    dev_t dev_id;
    struct cdev *chrdev;
    struct device *dev;
    struct class *pclass;
    struct buffer buf;
}virt_cdev;

// 虚拟字符设备列表
static struct virt_cdev *vcd_array = NULL;

static int vcd_open(struct inode *pinode, struct file *pfile)
{
    unsigned int minor = iminor(pinode);

    pr_info("open virt_cdev %d %d\n", imajor(pinode), iminor(pinode));
    pfile->private_data = (void*)(&vcd_array[minor-FIRST_MINOR_DEV_T].buf);

    return 0;
}

static int vcd_close(struct inode *pinode, struct file *pfile)
{
    pr_info("close virt_cdev %d %d\n", imajor(pinode), iminor(pinode));
    pfile->private_data = NULL;

    return 0;
}

loff_t vcd_llseek(struct file* pfile, loff_t offset, int whence)
{
    pr_info("llseek virt_cdev %d %d\n", imajor(pfile->f_inode), iminor(pfile->f_inode));

    struct buffer *buffer = (struct buffer*)pfile->private_data;
    loff_t new_offset = 0;

    switch(whence){
        case SEEK_SET:
            new_offset = offset < buffer->size ? offset : buffer->size-1;
            break;
        case SEEK_CUR:
            new_offset = pfile->f_pos + offset < buffer->size ? pfile->f_pos + offset : buffer->size-1;
            break;
        case SEEK_END:
            new_offset = buffer->size + offset < buffer->size ? buffer->size + offset : buffer->size-1;
            break;
        default:
            break;
    }

    if(new_offset < 0)
        return -EINVAL;
    
    pfile->f_pos = new_offset;
    return new_offset;
}

static ssize_t vcd_read(struct file *pfile, char __user *buff, size_t len, loff_t *offset)
{
    pr_info("read virt_cdev %d %d\n", imajor(pfile->f_inode), iminor(pfile->f_inode));

    struct buffer *buffer = (struct buffer*)pfile->private_data;
    unsigned long failed_len;

    len = (len <= buffer->size - *offset) ? len : buffer->size - *offset;
    failed_len = copy_to_user(buff, buffer->data, len);
    *offset += (len-failed_len);

    return (len-failed_len);
}

static ssize_t vcd_write(struct file *pfile, const char __user *buff, size_t len, loff_t *offset)
{
    pr_info("read virt_cdev %d %d\n", imajor(pfile->f_inode), iminor(pfile->f_inode));

    struct buffer *buffer = (struct buffer*)pfile->private_data;
    unsigned long failed_len;

    len = (len <= buffer->size - *offset) ? len : buffer->size - *offset;
    failed_len = copy_from_user(buffer->data + *offset, buff, len);
    *offset += (len - failed_len);

    return (len - failed_len);
}

static const struct file_operations vcd_ops = {
  .owner = THIS_MODULE,
  .open = vcd_open,
  .release = vcd_close,
  .llseek = vcd_llseek,
  .read = vcd_read,
  .write = vcd_write
};

static int buffer_alloc(void)
{
    int ret = 0;
    for(int i=0; i<VCD_COUNT; i++){
        vcd_array[i].buf.data = kzalloc(BUFFER_SIZE, GFP_KERNEL);
        if(!vcd_array[i].buf.data){
            while(i--){
                kfree(vcd_array[i].buf.data);
                vcd_array[i].buf.data = NULL;
                vcd_array[i].buf.size = 0;
            }
            ret = -ENOMEM;
            break;
        }
        vcd_array[i].buf.size = BUFFER_SIZE;
    }
    return ret;
}

static void buffer_free(void)
{
    for(int i=0; i<VCD_COUNT; i++){
        if(!vcd_array[i].buf.data){
            kfree(vcd_array[i].buf.data);
            vcd_array[i].buf.data = NULL;
            vcd_array[i].buf.size = 0;
        } 
    }
}

static char *vcd_devnode(const struct device *dev, umode_t *mode)
{
	if (mode)
		*mode = 0666;
	return NULL;
}

static int virt_cdev_register(struct platform_device *pdev)
{
    int ret = 0;

    // 分配vcd_array内存空间
    vcd_array = kzalloc(VCD_COUNT*sizeof(virt_cdev), GFP_KERNEL);
    if(!vcd_array){
        pr_err("Failed: kmalloc for vcd_array[]!\n");
        ret = -ENOMEM;
        goto failed_kmalloc_vcd;
    }

    // 分配cdev buffer内存空间
    ret = buffer_alloc();
    if(ret){
        pr_err("Failed: kmalloc for buffer!\n");
        goto failed_kmalloc_buff;
    }
        
    // 注册并分配设备号
    ret = alloc_chrdev_region(&FIRST_DEV_T, 0, VCD_COUNT, VCD_NAME);
    if(ret){
        pr_err("Failed: alloc_chrdev_region for virt_cdev!\n");
        goto failed_alloc_chrdev_region;
    }
    FIRST_MAJOR_DEV_T = MAJOR(FIRST_DEV_T);
    FIRST_MINOR_DEV_T = MINOR(FIRST_DEV_T);
    for(int i=0; i<VCD_COUNT; i++)
        vcd_array[i].dev_id = MKDEV(FIRST_MAJOR_DEV_T, FIRST_MINOR_DEV_T+i);

    // 初始化cdev
    cdev_init(&base_chrdev, &vcd_ops);
    base_chrdev.owner = THIS_MODULE;
    for(int i=0; i<VCD_COUNT; i++)
        vcd_array[i].chrdev = &base_chrdev;

    // 注册cdev
    ret = cdev_add(&base_chrdev, FIRST_DEV_T, 4);
    if(ret){
        pr_err("Failed: cdev_add for virt_cdev!\n");
        goto failed_cdev_add;
    }
    is_cdev_added = true;
    
    // 注册class
    VCD_CLASS = class_create("vcd_class");
    if(IS_ERR(VCD_CLASS)){
        pr_err("Failed: calss_create for virt_cdev!\n");
        ret = PTR_ERR(VCD_CLASS);
        goto failed_class_create;
    }
    VCD_CLASS->devnode = vcd_devnode; // 改变设备节点权限位
    for(int i=0; i<VCD_COUNT; i++)
        vcd_array[i].pclass = VCD_CLASS;

    // 注册设备节点
    for(int i=0; i<VCD_COUNT; i++){
        vcd_array[i].dev = device_create(vcd_array[i].pclass, &pdev->dev, 
                                vcd_array[i].dev_id, NULL, 
                                "%s-%d", VCD_NAME, MINOR(vcd_array[i].dev_id));
        if(!vcd_array[i].dev){
            while(i--){
                device_destroy(vcd_array[i].pclass, vcd_array[i].dev_id);
                vcd_array[i].dev = NULL;
            }
            pr_err("Failed: device_create for virt_cdev!\n");
            goto failed_device_create;
        }
    }

    pr_info("init sucessed!\n");
	return 0;

failed_device_create:
    class_destroy(VCD_CLASS);
    VCD_CLASS = NULL;
    for(int i=0; i<VCD_COUNT; i++)
        vcd_array[i].pclass = NULL;

failed_class_create:
    cdev_del(&base_chrdev);
    is_cdev_added = false;
    for(int i=0; i<VCD_COUNT; i++)
        vcd_array[i].chrdev = NULL;

failed_cdev_add:
    unregister_chrdev_region(FIRST_DEV_T, VCD_COUNT);

failed_alloc_chrdev_region:
    buffer_free();

failed_kmalloc_buff:
    kfree(vcd_array);
    vcd_array = NULL;

failed_kmalloc_vcd:
    return ret;
}

static void virt_cdev_unregister(void)
{
    for(int i=0; i<VCD_COUNT; i++){
        if(vcd_array[i].dev)
            device_destroy(vcd_array[i].pclass, vcd_array[i].dev_id);
    }

    if(VCD_CLASS)
        class_destroy(VCD_CLASS);
    
    if(is_cdev_added)
        cdev_del(&base_chrdev);

    if(FIRST_DEV_T)
        unregister_chrdev_region(FIRST_DEV_T, VCD_COUNT);
    
    buffer_free();
    kfree(vcd_array);
}


static int vcd_platform_probe(struct platform_device *pdev)
{
    pr_info("%s: %s\n", __func__, pdev->name);

    int ret = 0;
    ret = virt_cdev_register(pdev);
    return ret;
}

static int vcd_platform_remove(struct platform_device *pdev)
{
    virt_cdev_unregister();
    pr_info("%s: exit successed!\n", __func__);
    return 0;
}

static struct platform_device_id platform_id_table[] = {
    {.name = "vcd_platform_device"}
};

// platform驱动
static struct platform_driver vcd_platform_driver = {
    .probe = vcd_platform_probe,
    .remove = vcd_platform_remove,
    .driver = {
        .name = "vcd_platform_driver",
        .owner = THIS_MODULE
    },
    .id_table = platform_id_table
};

static struct platform_device *vcd_platform_device;

static int __init vcd_platform_init(void) {
    vcd_platform_device = platform_device_register_simple("vcd_platform_device", -1, NULL, 0);
    return platform_driver_register(&vcd_platform_driver);
}

static void __exit vcd_platform_exit(void) {
    platform_device_unregister(vcd_platform_device);
    platform_driver_unregister(&vcd_platform_driver);
}

module_init(vcd_platform_init);
module_exit(vcd_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zcj");
MODULE_DESCRIPTION("Virtual Char Device");
MODULE_VERSION("1.0");
MODULE_ALIAS("virtual_cdev");