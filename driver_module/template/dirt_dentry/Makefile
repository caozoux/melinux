PWD = $(shell pwd)  
KERSRC := /home/zc/github/cache/cloud-kernel/

TARGET := moniter_watchdog

obj-m := $(TARGET).o  
$(TARGET)-objs := dirt_dentry.o
all:
	#$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
	$(MAKE) -C ${KERSRC} M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/  *.mod -rf
	rm *.o -f
	rm .*.cmd -f
