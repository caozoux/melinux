PWD = $(shell pwd)  

TARGET := memtest

obj-m := $(TARGET).o  
$(TARGET)-objs := memory_entry.o
all:
	$(MAKE) -C $(KERBUILD) M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
