PWD = $(shell pwd)  

TARGET := meblock_test

obj-m := $(TARGET).o  
$(TARGET)-objs := blocktest_entry.o
all:
	$(MAKE) -C ${KERNEL_BUILD} M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
