PWD = $(shell pwd)
KERNEL_SRC = /fslink/ti/kernel-3.14.x
#KER_OUT = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x/out
TARGET := hiddrv

obj-m := $(TARGET).o
$(TARGET)-objs :=  hid_drver.o
all:
	#$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_SRC) O=$(KER_OUT) M=$(PWD) modules  
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -C $(KERNEL_SRC) M=$(PWD) modules  
	sudo cp $(TARGET).ko /var/lib/tftpboot/rootfs_altera/root/
		
clean:
	rm *.ko
	rm *.o
