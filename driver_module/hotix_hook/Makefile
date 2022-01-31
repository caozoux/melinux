PWD = $(shell pwd)  

TARGET := kpatch_hootk

KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build

EXTRA_CFLAGS += -I/home/zc/github/cloud-kernel

obj-m := $(TARGET).o  
$(TARGET)-objs := kpatchtest_entry.o
all:
	$(MAKE) -C $(KERSRC)  M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
