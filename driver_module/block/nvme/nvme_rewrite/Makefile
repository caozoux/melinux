PWD = $(shell pwd)  

KERVER =$(shell uname -r)
KERSRC := /export/disk512M/github/cache/linux-stable/
TARGET := nvme_retest

obj-m := $(TARGET).o  
$(TARGET)-objs := pci.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
