PWD = $(shell pwd)  

TARGET := kpatch2

obj-m := $(TARGET).o  
$(TARGET)-objs := kpatch2_entry.o
all:
	$(MAKE) -C /lib/modules/3.10.0-327.ali2011.alios7.x86_64/build M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
