# hello_md makefile
ifneq ($(KERNELRELEASE),)
obj-m += hello_cdev.o
hello_cdev-y := hello_device.o
else
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif