cflags += -I../ -I../module/include/
lflags += -lpthread
#lflags += -lgflags
#static_libs :=-static-libstdc++ -static-libgcc

CC := g++

all: kapp-tool


#kapp-tool: main.o rcu_test.o kmem.o block.o ktime.o  kprobe.c lock.o workqueue.o radixtree.o sched_main.o pci_unit.o panic_unit.o inject.o kdiagnose_unit.o
#kapp-tool: entry.o kinject.o kprobe.o ktrace.o
kapp-tool: entry.o kinject.o kmem.o kblock.o ksched.o krunlog.o kstack.o lib/symbole.o
	${CROSS_COMPILE}${CC} -L/usr/lib64 -lm -lc  $^ -o kapp-tool ${lflags}  $(static_libs)

%.o:%.cpp
	${CROSS_COMPILE}${CC} -c ${cflags} $^ -o $@

clean:
	@rm *.o kapp-tool
