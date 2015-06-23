PWD = $(shell pwd)  
#KERNEL_SRC = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x
#KER_OUT = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x/out
TARGET := virtualme_platform
KER_SRC_3_2 := /export/disk1T1/bsp_work/marvell/linux-stable-3.2.58
ROOTPATH := /var/lib/tftpboot/rootfs_marvall_xp/buildroot-2012.11-2014_T3.0-pj4b_armv7_le_sfp

obj-m := $(TARGET).o  
$(TARGET)-objs :=  virtual_bus.o virtual_dev.o
all:
	#$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_SRC) O=$(KER_OUT) M=$(PWD) modules  
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KER_SRC_3_2)  M=$(PWD) modules  
	sudo cp $(TARGET).ko $(ROOTPATH)/root
		
clean:
	@rm -f *.ko
	@rm -f *.o modules.* Module.symvers *.mod.c
