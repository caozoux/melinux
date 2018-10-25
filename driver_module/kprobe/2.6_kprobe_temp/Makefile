PWD = $(shell pwd)  
BUILD=/lib/modules/$(KERVER)/build/
TARGET := 26_kprobe_test

obj-m := $(TARGET).o  
$(TARGET)-objs := kprobe_entry.o
all:
	$(MAKE) -C /lib/modules/`uname -r`/build/   M=$(PWD) modules
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c -f
	rm *.o -f
	rm *.ko.cmd *.o.cmd .tmp_versions/ -rf
