PWD = $(shell pwd)  

TARGET := module_template

obj-m := $(TARGET).o  
$(TARGET)-objs := example_kprobe.o
all:
	$(MAKE) -C /lib/modules/`uname -r`/build/ M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/  *.mod -rf
	rm *.o -f
	rm .*.cmd -f
