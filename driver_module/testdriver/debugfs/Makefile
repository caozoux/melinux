PWD = $(shell pwd)  
TARGET := medebugfs

obj-m := $(TARGET).o  
$(TARGET)-objs := medebugfs_entry.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko
	rm *.o
