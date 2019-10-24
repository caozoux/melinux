PWD = $(shell pwd)  

PWD = $(shell pwd)  

KERVER =$(shell uname -r)
KERSRC :=/lib/modules/$(KERVER)/build

TARGET := me_test

obj-m := $(TARGET).o  
$(TARGET)-objs := test_entry.o
all:
	$(MAKE) -C $(KERSRC) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
