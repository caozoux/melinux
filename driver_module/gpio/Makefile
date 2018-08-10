PWD = $(shell pwd)  
obj-m := melinux_gpio.o  
module-objs := melin.o  
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  

		
clean:
	rm *.ko
	rm *.o
