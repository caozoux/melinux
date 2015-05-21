PWD = $(shell pwd)  
#KERNEL_SRC = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x
#KER_OUT = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x/out
KERNEL_SRC = /export/disk1T1/bsp_work/wrlinux/wrlinux6/prj/ti-am335x/build/linux-windriver/linux
KER_OUT = /export/disk1T1/bsp_work/wrlinux/wrlinux6/prj/ti-am335x/build/linux-windriver/linux-ti-am335x-preempt-rt-build
TARGET := medma_test

obj-m := $(TARGET).o  
$(TARGET)-objs := dmatest_entry.o
all:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_SRC) O=$(KER_OUT) M=$(PWD) modules  
	sudo cp $(TARGET).ko  /var/lib/tftpboot/dist/root/
		
clean:
	rm *.ko
	rm *.o
