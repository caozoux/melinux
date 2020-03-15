KERSRC := /lib/modules/`uname -r`/build
PWD = $(shell pwd)  

TARGET := nvme_core_test

obj-m := $(TARGET).o  
$(TARGET)-objs := core.o configfs.o admin-cmd.o fabrics-cmd.o discovery.o io-cmd-file.o io-cmd-bdev.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
