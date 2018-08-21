PWD = $(shell pwd)  

TARGET := slubufa

obj-m := $(TARGET).o  
$(TARGET)-objs := slubufa_entry.o
all:
	$(MAKE) -C $(KERSRC)  M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
