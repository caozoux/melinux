KERNELVER ?= $(shell uname -r)
KERNEL_BUILD_PATH ?= /lib/modules/$(KERNELVER)/build

nvme:
	$(MAKE) -C $(KERNEL_BUILD_PATH) M=`pwd`/xfs

clean:
	$(MAKE) -C $(KERNEL_BUILD_PATH) M=`pwd`/xfs clean

headers_install:
	@ cp include/uapi/linux/* /usr/include/linux/

.PHONY: all clean nvme headers_install

