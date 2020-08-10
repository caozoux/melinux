PWD = $(shell pwd)  

TARGET := misc

KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build
obj-m := $(TARGET).o  
$(TARGET)-objs := misc_entry.o page_test.o rcu_test.o kprobe_entry.o showstack.o workqueue_test.o delay.o locktest_entry.o msr_arm.o raidtree_test.o block/ext2_test.o atomic_entry.o devbusdrv_unit.o kmem/kmem_unit.o hw/pci/pci_unit.o
all:
	make -C user_space
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
