PWD = $(shell pwd)  

TARGET := kpatch_hootk

obj-m := $(TARGET).o  
$(TARGET)-objs := kpatchtest_entry.o
all:
	$(MAKE) -C /export/disk1T/zoucao.zcdisk/cach/7u  M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
