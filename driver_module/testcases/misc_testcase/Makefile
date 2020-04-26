PWD = $(shell pwd)  

TARGET := misc

KERVER =$(shell uname -r)
KERSRC :=/lib/modules/$(KERVER)/build
obj-m := $(TARGET).o  
$(TARGET)-objs := misc_entry.o page_test.o mem_test.o rcu_test.o
all:
	make -C user_space
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
