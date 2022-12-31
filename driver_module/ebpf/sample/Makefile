.PHONY: all
# /home/zc/github/kernel-4.18.0
KERHSRC ?= /usr/src/kernels/4.18.0-408.el8.x86_64/
ebpf_mod := ebpf_module_kprobe.o
#test : sudo ./namespace_execute_child -p -m  ./namespace_init

all: EBPF_MOD ebpf_loader

EBPF_MOD: $(ebpf_mod)
ebpf_loader:
	clang -DHAVE_ATTR_TEST=0 -o monitor-exec -lelf -lbpf  \
	-I$(KERHSRC)/samples/bpf \
	-I$(KERHSRC)/tools/lib \
	-I$(KERHSRC)/tools/perf \
	-I$(KERHSRC)/tools/include \
	bpf_load.c ebpf_main.c
#	$(KERSRC)/samples/bpf/bpf_load.c ebpf_loader.c

%.o:%.c
	clang  -nostdinc -isystem /usr/lib/gcc/x86_64-redhat-linux/8/include -I${KERHSRC}/arch/x86/include -I./arch/x86/include/generated  -I${KERHSRC}/include/drm-backport  -I${KERHSRC}/include -I./include/drm-backport -I./include -I${KERHSRC}/arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I${KERHSRC}/include/uapi -I./include/generated/uapi -include ${KERHSRC}/include/linux/kconfig.h  -g -I${KERHSRC}/samples/bpf \
	-I/usr/src/kernels/4.18.0-408.el8.x86_64/arch/x86/include/generated/ \
	-I/usr/src/kernels/4.18.0-408.el8.x86_64/arch/x86/include/generated/uapi \
	-I/usr/src/kernels/4.18.0-408.el8.x86_64/include/generated/uapi \
	-D__KERNEL__ -D__BPF_TRACING__ -Wno-unused-value -Wno-pointer-sign \
	-D__TARGET_ARCH_x86 -Wno-compare-distinct-pointer-types \
	-Wno-gnu-variable-sized-type-not-at-end \
	-Wno-address-of-packed-member -Wno-tautological-compare \
	-Wno-unknown-warning-option  \
	-I/home/zc/github/kernel-4.18.0/samples/bpf/ -include asm_goto_workaround.h \
	-O2 -emit-llvm -c $< -o -| llc -march=bpf $(LLC_FLAGS) -filetype=obj -o $@

clean:
	@rm *.o
	@rm multiple_map   simple_map

