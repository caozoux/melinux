KERSRC ?= /lib/modules/`uname -r`/build/
PWD = $(shell pwd)  

TARGET := misc_device

obj-m := $(TARGET).o  
$(TARGET)-objs := entry.o
all:
	$(MAKE) -C ${KERSRC} M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/  *.mod -rf
	rm *.o -f
	rm .*.cmd -f
