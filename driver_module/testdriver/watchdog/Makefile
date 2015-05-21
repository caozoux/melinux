PWD = $(shell pwd)  
KERNEL_SRC = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x
KER_OUT = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x/out
TARGET := mewdg_test

obj-m := $(TARGET).o  
$(TARGET)-objs := watchdogtest.o
all:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_SRC) O=$(KER_OUT) M=$(PWD) modules  
	sudo cp $(TARGET).ko  /var/lib/tftpboot/dist/root/
		
clean:
	rm *.ko
	rm *.o
