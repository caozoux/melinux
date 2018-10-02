PWD = $(shell pwd)  

TARGET := page

obj-m := $(TARGET).o  
$(TARGET)-objs := page_entry.o
all:
	$(MAKE) -C /lib/modules/3.10.0-862.el7.x86_64/build/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
