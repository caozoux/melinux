#include "../vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include "../template.h"

SEC("tp/vmscan/mm_vmscan_wakeup_kswapd")
int mm_vmscan_wakeup_kswapd(void *ctx)
{
	return 0;
}
