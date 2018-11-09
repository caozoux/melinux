PWD = $(shell pwd)  

TARGET := slub

obj-m := $(TARGET).o  
$(TARGET)-objs := slub_entry.o slubufa_entry.o kmemcache.o kmemcache_rcu.o
all:
	$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
