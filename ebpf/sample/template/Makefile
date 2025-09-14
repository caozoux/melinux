target := template

SRCS = template.c stacktrace.c
OBJS = $(SRCS:.c=.o)
CFLAGS :=-I./

all: bpf/${target}.bpf.o bpf/${target}.skel.h ${target}

#vmlinux:
#	bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

bpf/${target}.bpf.o: bpf/${target}.bpf.c
	clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -c $< -o $@

bpf/${target}.skel.h: bpf/${target}.bpf.o
	bpftool gen skeleton $< > $@

%.o: %.c
	gcc -Wall -c $< -o $@

#${target}: ${target}.c bpf/${target}.skel.h
#	gcc -Wall -o $@ $< -lbpf
${target}: $(OBJS) bpf/${target}.bpf.o
	 $(CC) $(CFLAGS) $(OBJS) -o $@ -lbpf
	 #$(CC) $(CFLAGS) $(OBJS) -o $@ /usr/lib64/libbpf.a -lelf -lz

.PHONY:
clean:
	rm -rf bpf/${target}.bpf.o bpf/${target}.skel.h ${target} *.o
