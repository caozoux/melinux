PWD = $(shell pwd)  

TARGET := ramfsv2

obj-m := $(TARGET).o  
$(TARGET)-objs := inode.o file-mmu.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
