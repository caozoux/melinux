PWD = $(shell pwd)  

TARGET := tcp_tools
KERVER  = $(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build


ccflags-y += -I$(KERHSRC)
obj-m := $(TARGET).o  
$(TARGET)-objs := entry.o
all:
	$(MAKE) -C $(KERSRC) M=$(PWD) modules
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
