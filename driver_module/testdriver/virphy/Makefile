PWD = $(shell pwd)  
#KERNEL_SRC = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x
#KER_OUT = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x/out
KERNEL_SRC = /fslink/ti/kernel-3.14.x
#KER_OUT = /export/disk1T1/bsp_work/wrlinux/wrlinux6/prj/ti-am335x/build/linux-windriver/linux-ti-am335x-preempt-rt-build
TARGET := mdiophy_vir

obj-m := $(TARGET).o  
$(TARGET)-objs := virture_phy.o
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  
	sudo cp mdiophy_vir.ko  /var/lib/tftpboot/rootfs_ls1021/root/
		
clean:
	rm *.ko
	rm *.o
