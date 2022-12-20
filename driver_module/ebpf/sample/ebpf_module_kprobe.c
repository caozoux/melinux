#include <linux/bpf.h>
#include <linux/version.h>
#define SEC(NAME) __attribute__((section(NAME), used))

static int (*bpf_trace_printk)(const char *fmt, int fmt_size,
                              ...) = (void *)BPF_FUNC_trace_printk;
//
#if 0
SEC("tracepoint/syscalls/sys_enter_execve")
int bpf_prog(void *ctx) {
  char msg[] = "Hello, eBPF World!";
  bpf_trace_printk(msg, sizeof(msg));
  return 0;
}
#endif
SEC("kprobe/__netif_receive_skb_core")
int bpf_prog1(struct pt_regs *ctx)
{

	return 0;
}

char _license[] SEC("license") = "GPL";
unsigned int _version SEC("version") = LINUX_VERSION_CODE;
