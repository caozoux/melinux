#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

SEC("kprobe/vfs_write")
int test_code1(struct xdp_md *ctx)
{
	char fmt[] = "zz len %d\n";
	bpf_trace_printk(fmt, sizeof(fmt), 11);
	return 0;
}

char _license[] SEC("license") = "GPL";
