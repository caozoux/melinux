PWD = $(shell pwd)  

TARGET := misc

obj-m := $(TARGET).o  
$(TARGET)-objs := workqueue_entry.o workquere_base.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
