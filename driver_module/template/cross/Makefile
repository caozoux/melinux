PWD = $(shell pwd)  

TARGET := test

obj-m := $(TARGET).o  
$(TARGET)-objs := testmodule.o
all:
	$(MAKE) ARCH=arm64 -C $KERSRC M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
