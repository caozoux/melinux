PWD = $(shell pwd)  

TARGET := clocksource

obj-m := $(TARGET).o  
$(TARGET)-objs := clocksource_entry.o
all:
	$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
