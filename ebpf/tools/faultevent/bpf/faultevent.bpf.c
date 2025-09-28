#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include "../faultevent.h"

#define PERF_MAX_STACK_DEPTH 32
#define KERN_STACKID_FLAGS  (0 | BPF_F_FAST_STACK_CMP)
#define _(P) ({                     \
    typeof(P) val;                  \
    __builtin_memset(&val, 0, sizeof(val));     \
    bpf_probe_read(&val, sizeof(val), &P);      \
    val;                        \
})


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

static report_event(enum FAULT_EVENT type, void *ctx)
{
	report event={};
	event.type =  type;
	bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
			&event, sizeof(report));
}

SEC("tp/block/block_rq_error")
int trace_block_rq_error(void *ctx)
{
	report_event(FE_IO_ERR, ctx);
	return 0;
}

SEC("kprobe/out_of_memory")
int kprobe_out_of_memory(struct pt_regs *ctx)
{
	struct oom_control *oc = (struct oom_control *)ctx->di;

	if (oc->memcg) {
		report_event(FE_OOM_CGROUP, ctx);
	} else {
		report_event(FE_OOM_GLOBAL, ctx);
	}

	return 0;
}

SEC("tp/sched/sched_process_hang")
int trace_sched_process_hang(void *ctx)
{
	report_event(FE_HUNGTASK, ctx);
	return 0;
}

SEC("tp/tcu/rcu_stall_warning")
int trace_rcu_stall_warning(void *ctx)
{
	report_event(FE_RCUSTALL, ctx);
	return 0;
}

SEC("kprobe/add_taint")
int kprobe_add_taint(struct pt_regs *ctx)
{
	unsigned int flag = (unsigned int) ctx->di;
	if (flag == TAINT_SOFTLOCKUP)
	return 0;
}

SEC("kprobe/warn_alloc")
int kprobe_warn_alloc(struct pt_regs *ctx)
{
	report_event(FE_ALLOCFAIL, ctx);
	return 0;
}

