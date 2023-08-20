PWD = $(shell pwd)  

TARGET := mutex_trace

EXTRA_CFLAGS := -I/home/zc/github/kernel-4.18.0/
obj-m := $(TARGET).o  
$(TARGET)-objs := entry.o
all:
	#$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
	$(MAKE) -C /work/cache/kernel-4.18.0/  M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/  *.mod -rf
	rm *.o -f
	rm .*.cmd -f
