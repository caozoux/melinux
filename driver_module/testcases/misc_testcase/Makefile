CUR_PATH = $(shell pwd)

TARGET := misc

KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build

obj-m := $(TARGET).o
#KBUILD_CFLAGS
ccflags-y += -I$(PWD)/include -I$(PWD)/$(PRIV_INC) -I$(PWD)/../ -Wuninitialized 

$(TARGET)-objs := misc_entry.o page_test.o rcu_test.o kprobe_entry.o showstack.o workqueue_test.o delay.o locktest_entry.o msr_arm.o raidtree_test.o block/ext2_test.o atomic_entry.o devbusdrv_unit.o kmem/kmem_unit.o hw/pci/pci_unit.o statickey_unit.o sched_unit.o cpu_unit.o block_unit.o 

ifdef CONFIG_ARM64
$(TARGET)-objs += arm64/gic.o
endif

$(TARGET)-objs += kernel/task.o kernel/dentry.o block/request.o

all:
	make -C user_space
	echo $(CUR_PATH) 
	$(MAKE) -C $(KERSRC) M=$(PWD) modules
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
