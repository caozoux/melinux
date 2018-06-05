PWD = $(shell pwd)  
TARGET := waitqueue_test

obj-m := $(TARGET).o  
$(TARGET)-objs := waitqueue_entry.o
all:
	$(MAKE) -C $(KERSRC) O=$(KEROUT) M=$(PWD) modules  
		
clean:
	rm *.ko
	rm *.o
