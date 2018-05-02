PWD = $(shell pwd)  
KERNEL_SRC = /home/zoucao.zc/github/alikernel/7u/
obj-m := pmu_entry.o  
module-objs := pmu_test.o  
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  

		
clean:
	rm *.ko
	rm *.o
