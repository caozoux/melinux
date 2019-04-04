PWD = $(shell pwd)  

TARGET := dmatest

obj-m := $(TARGET).o  
$(TARGET)-objs := dma_entry.o dma_alloc.o
all:
	$(MAKE) -C /export/disk1T/zoucao.zcdisk/cach/7u M=$(PWD) modules  
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
