KERSRC := /lib/modules/`uname -r`/build
PWD = $(shell pwd)  

TARGET := nvme_test

obj-m := $(TARGET).o  
$(TARGET)-objs := pci.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
