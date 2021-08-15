PWD = $(shell pwd)
KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$KERVER)/build
KERSRC ?=/export/disk1T/zoucao.zcdisk/cach/ck/

TARGET := menvme

obj-m := $(TARGET).o
$(TARGET)-objs := nvme_pci.o nvme_block.o nvme_spec.o
all:
	$(MAKE) -C $(KERSRC) M=$(PWD) modules

clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f

