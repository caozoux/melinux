PWD = $(shell pwd)

TARGET := cpuid_test

obj-m := $(TARGET).o
$(TARGET)-objs := cpuid_entry.o cpuboot_data.o
all:
	$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
