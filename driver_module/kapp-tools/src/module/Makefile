CUR_PATH = $(shell pwd)

TARGET := ksys_tool

KERVER =$(shell uname -r)
KERSRC ?= /lib/modules/$(KERVER)/build

obj-m := $(TARGET).o

#KBUILD_CFLAGS
ccflags-y += -I$(PWD)/include -I$(PWD)/$(PRIV_INC) -I$(PWD)/../ -I$(PWD)/lib -Wuninitialized -I$(KERHSRC) -I$(PWD)/kdiagnose/include -I$(KERSOURCE)

$(TARGET)-objs := kapp_entry.o

#$(TARGET)-objs += ./lib/page.o ./lib/raidtree.o ./lib/delay.o

ifdef CONFIG_ARM64
#$(TARGET)-objs += arm64/gic/gic_entry.o arm64/gic/gic_virtual_device.o
endif

$(TARGET)-objs += kprobe/kprobe_unit.o
$(TARGET)-objs += base/base.o base/percpu_variable.o base/trace.o base/buffer.o base/runlog.o base/trace_buffer.o  \
				  base/slab.o base/stack.o

$(TARGET)-objs += ktrace/ktrace_unit.o
$(TARGET)-objs += kinject/kinject_test.o kinject/kinject_unit.o  kinject/kinject_timer.o  \
				  kinject/kinject_slub.o kinject/kinject_sem.o kinject/kinject_segment.o \
				  kinject/kinject_lock.o kinject/kinject_thread.o
$(TARGET)-objs += kblock/kblock_unit.o kblock/super.o  kblock/bio.o  kblock/dentry.o  kblock/inode.o  kblock/request.o kblock/kblock_trace.o

$(TARGET)-objs += kmem/kmem_dump.o kmem/kmem_unit.o kmem/cgroup/cgroup.o \
				  kmem/slab/slab.o

$(TARGET)-objs += kdevice/kdevice_unit.o
$(TARGET)-objs += kstack/kstack_unit.o
$(TARGET)-objs += ktree_list/ktree_list_unit.o
$(TARGET)-objs += ksched/ksched_unit.o ksched/ksched_domain.o ksched/ksched_montor.o

all:
	$(MAKE) -C $(KERSRC) M=$(PWD) modules
	scp *.ko root@vm:~/ && scp ../app/kapp-tool  root@vm:~/  && ssh root@vm "rmmod ksys_tool.ko  && insmod ksys_tool.ko"
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f

