PWD = $(shell pwd)  
KERNEL_SRC = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x
obj-m := melinux_gpio.o  
module-objs := test.o  
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  

		
clean:
	rm *.ko
	rm *.o
