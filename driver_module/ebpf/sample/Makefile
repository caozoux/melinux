.PHONY: all

KERHSRC := /home/zc/github/cloud-kernel
ebpf_mod := ebpf_module_kprobe.o
#test : sudo ./namespace_execute_child -p -m  ./namespace_init

all: EBPF_MOD ebpf_loader

#rcu_read_targ: rcu_read_targ.o
#rcu_write_targ: rcu_write_targ.o
#sample_target: ebpf_main.o
#	gcc -o $@ $< -lpthread
EBPF_MOD: $(ebpf_mod)
ebpf_loader:
	clang -DHAVE_ATTR_TEST=0 -o monitor-exec -lelf -lbpf  \
	-I$(KERHSRC)/samples/bpf \
	-I$(KERHSRC)/tools/lib \
	-I$(KERHSRC)/tools/perf \
	-I$(KERHSRC)/tools/include \
	bpf_load.c ebpf_loader.c
#	$(KERSRC)/samples/bpf/bpf_load.c ebpf_loader.c


#$(ebpf_main):%.o:%.c
%.o:%.c
	 clang  -I/usr/include/x86_64-linux-gnu -DHAVE_ATTR_TEST=0 -O2 -target bpf -c $<  -o $@

clean:
	@rm *.o
	@rm multiple_map   simple_map

