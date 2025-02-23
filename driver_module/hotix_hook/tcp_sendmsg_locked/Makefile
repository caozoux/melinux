PWD = $(shell pwd)  

TARGET := hookdebug

KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build

#EXTRA_CFLAGS += -I/home/zc/github/linux-stable/

obj-m := $(TARGET).o  
$(TARGET)-objs := entry.o
all:
	$(MAKE)  -C /export/disk1T/github/cloud-kernel/ O=/export/disk1T/github/cache/cloud-kernel  M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
