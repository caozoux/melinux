PWD = $(shell pwd)  
BUILD=/lib/modules/$(KERVER)/build/
TARGET := hook_chow

obj-m := $(TARGET).o  
$(TARGET)-objs := hook_chow_comm_entry.o
all:
	$(MAKE) -C /lib/modules/`uname -r`/build/   M=$(PWD) modules
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c -f
	rm *.o -f
	rm *.ko.cmd *.o.cmd .tmp_versions/ -rf
