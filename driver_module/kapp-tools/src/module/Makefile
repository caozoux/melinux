CUR_PATH = $(shell pwd)

TARGET := ksys_tool

KERVER =$(shell uname -r)
#KERSRC ?= /lib/modules/$(KERVER)/build
KERSRC ?= /home/zc/github/cache/chushi-linux/

obj-m := $(TARGET).o

#KBUILD_CFLAGS
ccflags-y += -I$(PWD)/include -I$(PWD)/$(PRIV_INC) -I$(PWD)/../ -I$(PWD)/lib -Wuninitialized -I$(KERHSRC) -I$(PWD)/kdiagnose/include

$(TARGET)-objs := kapp_entry.o

#$(TARGET)-objs += ./lib/page.o ./lib/raidtree.o ./lib/delay.o

ifdef CONFIG_ARM64
#$(TARGET)-objs += arm64/gic/gic_entry.o arm64/gic/gic_virtual_device.o
endif

$(TARGET)-objs += kprobe/kprobe_unit.o
$(TARGET)-objs += base/base.o base/percpu_variable.o
$(TARGET)-objs += ktrace/ktrace_unit.o


all:
	$(MAKE) -C $(KERSRC) M=$(PWD) modules
	scp *.ko root@vm:~/ && scp ../app/kapp-tool  root@vm:~/  && ssh root@vm "rmmod ksys_tool.ko  && insmod ksys_tool.ko"
		
clean:
	rm *.ko modules.order  Module.symvers *.mod.c *.ko.cmd *.o.cmd .tmp_versions/ -rf
	rm *.o -f

