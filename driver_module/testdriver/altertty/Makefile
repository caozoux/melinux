KERNEL_SRC = /fslink/ti/kernel-4.1.x
#KER_OUT = /export/disk1T1/bsp_work/TI_AM335X/kernel-3.14.x/out
TARGET := altera_uart

obj-m := altera_uart.o  
#test-objs := memem_dir.o
all:
	$(MAKE) -C $(KERNEL_SRC)  M=$(PWD) modules  
	@sudo cp $(TARGET).ko  /var/lib/tftpboot/rootfs_ls1021/root/
		
clean:
	@rm -f *.ko
	@rm -f *.o modules.* Module.symvers *.mod.c
