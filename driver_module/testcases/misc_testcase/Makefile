CUR_PATH = $(shell pwd)

TARGET := misc

KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build
KERSRC ?=/export/disk1T/zoucao.zcdisk/cach/ck/

obj-m := $(TARGET).o

#KBUILD_CFLAGS
ccflags-y += -I$(PWD)/include -I$(PWD)/$(PRIV_INC) -I$(PWD)/../ -I$(PWD)/lib -Wuninitialized -I$(KERHSRC)

$(TARGET)-objs := misc_entry.o page_test.o rcu_test.o kprobe_entry.o showstack.o workqueue_test.o delay.o lock_unit.o msr_arm.o radixtree_unit.o block/ext2_test.o atomic_entry.o devbusdrv_unit.o  hw/pci/pci_unit.o statickey_unit.o  cpu_unit.o block_unit.o cpu_unit.o ktime_unit.o

$(TARGET)-objs += ./lib/page.o
$(TARGET)-objs += ./lib/raidtree.o

ifdef CONFIG_ARM64
$(TARGET)-objs += arm64/gic/gic_entry.o arm64/gic/gic_virtual_device.o
endif

$(TARGET)-objs += kernel/task.o kernel/dentry.o kernel/virtual_bus.o  kernel/track.o
$(TARGET)-objs += block/request.o  block/block_disck.o block/inode.o  block/bio.o block/buffer_head.o block/readwrite.o
$(TARGET)-objs += kmem/kmem_unit.o kmem/page.o kmem/zone.o kmem/kmemcache.o kmem/resource.o
$(TARGET)-objs += ksched/ksched_unit.o  ksched/ksched_debug.o 
$(TARGET)-objs += ktree/ktree_rbtree_unit.o
$(TARGET)-objs += pci/pci_unit.o
$(TARGET)-objs += efi/efi_unit.o
$(TARGET)-objs += panic_hang/panic_unit.o


all:
	make -C user_space
	echo $(CUR_PATH) 
	$(MAKE) -C $(KERSRC) M=$(PWD) modules
	scp *.ko root@vm:~/ && scp user_space/test root@vm:~/  && ssh root@vm "rmmod misc.ko  && insmod misc.ko"
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f
	rm .*.cmd -f
