PWD = $(shell pwd)  

TARGET := pr_debug

obj-m := $(TARGET).o  
$(TARGET)-objs := prdebug_entry.o
all:
	$(MAKE) -C /export/disk1T/zoucao.zcdisk/hotfix/kernel-devel/usr/src/kernels/3.10.0-327.ali2016.alios7.x86_64/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
