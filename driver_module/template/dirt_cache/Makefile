PWD = $(shell pwd)  

TARGET := moniter_watchdog

obj-m := $(TARGET).o  
$(TARGET)-objs := dirt_cache.o trace.o
all:
	#$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
	$(MAKE) -C /home/zc/github/cache/kernel-4.18  M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/  *.mod -rf
	rm *.o -f
	rm .*.cmd -f
