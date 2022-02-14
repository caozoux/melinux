cflags += -I../

all: block_test test

block_test: block_main.c
	@gcc -L/usr/lib64 -lm -lc -lpthread  $^ -o block_test

test: main.o rcu_test.o kmem.o block.o ktime.o  kprobe.c lock.o workqueue.o radixtree.o sched_main.o pci_unit.o panic_unit.o inject.o
	@gcc -L/usr/lib64 -lm -lc -lpthread  $^ -o test

%.o:%.c
	@gcc -c ${cflags} $^

clean:
	@rm *.o
	@rm test
