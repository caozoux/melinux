PWD = $(shell pwd)  
#KERNEL_SRC = /fslink/ti/kernel-4.1.x
KERNEL_SRC = /fslink/ti/kernel-3.14.x
#KER_OUT = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x/out
TARGET := g_usbdrv

obj-m := $(TARGET).o  
$(TARGET)-objs := generic_usbdriver.o
all:
	#$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_SRC) O=$(KER_OUT) M=$(PWD) modules  
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -C $(KERNEL_SRC) M=$(PWD) modules  
	sudo cp $(TARGET).ko /var/lib/tftpboot/rootfs_ls1021/root/
		
clean:
	rm *.ko
	rm *.o
