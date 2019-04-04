PWD = $(shell pwd)  
TARGET := kprobe_ping

obj-m := $(TARGET).o  
$(TARGET)-objs := kprobe_entry.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c -f
	rm *.o -f
	rm *.ko.cmd *.o.cmd .tmp_versions/ -rf
