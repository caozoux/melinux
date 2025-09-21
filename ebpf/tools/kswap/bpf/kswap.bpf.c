#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include "../kswap.h"

#define PERF_MAX_STACK_DEPTH 32
#define KERN_STACKID_FLAGS  (0 | BPF_F_FAST_STACK_CMP)
#define _(P) ({                     \
    typeof(P) val;                  \
    __builtin_memset(&val, 0, sizeof(val));     \
    bpf_probe_read(&val, sizeof(val), &P);      \
    val;                        \
})

typedef struct {
	//rwsem start to get lock
    u64 start_ts;
	//rwsem get lock
    u64 get_ts;
	//wake the rwsem task
    u64 wake_ts;
} report;

struct {
    __uint(type, BPF_MAP_TYPE_STACK_TRACE);
	__uint(key_size, sizeof(u32));
	__uint(value_size, PERF_MAX_STACK_DEPTH * sizeof(u64));
    __uint(max_entries, 1000);
}stackmap SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, report);
} start SEC(".maps");

/*
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 2);
	__type(key, u32);
	__type(value, struct arg_info);
} arg_map SEC(".maps");
*/

struct {
	__uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
	__uint(key_size, sizeof(u32));
	__uint(value_size, sizeof(u32));
}events SEC(".maps");

SEC("tp/vmscan/mm_vmscan_wakeup_kswapd")
int mm_vmscan_wakeup_kswapd(void *ctx)
{
	return 0;
}
