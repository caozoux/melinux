PWD = $(shell pwd)  

TARGET := melivepatch_test
KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build

obj-m := $(TARGET).o  
$(TARGET)-objs := livepatch.o
all:
	$(MAKE)  -C $(KERSRC) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
