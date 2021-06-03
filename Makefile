ifneq ($(KERNELRELEASE),)
	obj-m += dummy_module.o
else
	TARGET_MODULE := dummy_module
	KERNEL_FOLDER ?= /lib/modules/$(shell uname -r)/build
	PWD = $(shell pwd)
all:
	compiledb $(MAKE) -C $(KERNEL_FOLDER) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_FOLDER) M=$(PWD) clean
load:
	insmod ./$(TARGET_MODULE).ko
unload:
	rmmod ./$(TARGET_MODULE).ko
ioctl: ioctl.c
	gcc ioctl.c -o ioctl
endif	
