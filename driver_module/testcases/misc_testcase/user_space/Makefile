cflags += -I../

all: block_test test

block_test: block_main.c
	@${CROSS_COMPILE}gcc -L/usr/lib64 -lm -lc -lpthread  $^ -o block_test

test: main.o rcu_test.o kmem.o block.o ktime.o  kprobe.c lock.o workqueue.o radixtree.o sched_main.o pci_unit.o panic_unit.o inject.o kdiagnose_unit.o
	@${CROSS_COMPILE}gcc -L/usr/lib64 -lm -lc -lpthread  $^ -o test

%.o:%.c
	@${CROSS_COMPILE}gcc -c ${cflags} $^

clean:
	@rm *.o
	@rm test
