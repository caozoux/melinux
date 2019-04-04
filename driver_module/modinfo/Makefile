PWD = $(shell pwd)  

TARGET := medma_test

obj-m := $(TARGET).o  
$(TARGET)-objs := gpiotest.o
all:
	$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
