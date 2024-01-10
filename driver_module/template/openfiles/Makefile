PWD = $(shell pwd)  

TARGET := module_template

obj-m := $(TARGET).o  
$(TARGET)-objs := entry.o
all:
	#$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
	$(MAKE) -C /lib/modules/4.18.0-2.4.3.kwai.x86_64/build/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
