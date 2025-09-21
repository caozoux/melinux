#include <linux/version.h>
#include <string.h>
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_tracing.h>
#include "../lockdetect.h"

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
	//rwsem free lock
    u64 end_ts;
	u64 throttled_time_start;
	u64 throttled_time_end;
    u64 lock;
	int switch_cnt;
} lock_data;

struct sched_sched_wakeup_args {
    struct trace_entry ent;
    char prev_comm[16];
    pid_t pid;
    int prio;
	int target_cpu;
};

struct bpf_map_def SEC("maps") stackmap = {
    .type = BPF_MAP_TYPE_STACK_TRACE,
    .key_size = sizeof(u32),
    .value_size = PERF_MAX_STACK_DEPTH * sizeof(u64),
    .max_entries = 1000,
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, lock_data);
} start SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 2);
	__type(key, u32);
	__type(value, struct arg_info);
} arg_map SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
	__uint(key_size, sizeof(u32));
	__uint(value_size, sizeof(u32));
}events SEC(".maps");

enum lock_type {
	LOCK_TYPE_NONE,
	LOCK_RWSEM_READELOCK_ENTRY,
	LOCK_RWSEM_READLOCK_EXIT,
	LOCK_RWSEM_READUNLOCK_ENTRY,
	LOCK_RWSEM_WRITEELOCK_ENTRY,
	LOCK_RWSEM_WRITELOCK_EXIT,
	LOCK_RWSEM_WRITEUNLOCK_ENTRY,
	LOCK_MUTEX_LOCK_ENTRY,
	LOCK_MUTEX_LOCK_EXIT,
	LOCK_MUTEX_UNLOCK_ENTRY,
};

static int lock_entry(u64 addr, enum lock_type type)
{
	u32 pid;
	lock_data lock = {};
	int i  = 0;
	struct filter  filter;
	struct arg_info  *argp;

    argp = bpf_map_lookup_elem(&arg_map, &i);
    if (argp)
        filter = _(argp->filter);
    else
        return 0;

	if (filter.addr && filter.addr != (u64)addr)
		return 0;

	pid = bpf_get_current_pid_tgid();
	lock.start_ts = bpf_ktime_get_ns();
	lock.wake_ts = 0;
	lock.get_ts= 0;
	lock.end_ts= 0;

	bpf_map_update_elem(&start, &pid, &lock, 0);

	return 0;
}

static int lock_exit(u64 addr, enum lock_type type, u64 *ctx)
{
	u32 pid = bpf_get_current_pid_tgid();
	lock_data *lock;
	int i  = 0;
	u64 ts = bpf_ktime_get_ns();
	struct task_struct *p;
	struct task_group  *sched_task_group;
	struct filter  filter;
	struct arg_info  *argp;

    argp = bpf_map_lookup_elem(&arg_map, &i);
    if (argp)
        filter = _(argp->filter);
    else
        return 0;

	if (filter.addr && filter.addr != (u64)addr)
		return 0;

	lock = bpf_map_lookup_elem(&start, &pid);
	if (!lock)
		return 0;

	lock->get_ts = ts;

	if (filter.mode == LOAD_RWSEM || filter.mode == LOAD_MUTEX ) {
		u64 delta;
		lock_report_data report = {};

		delta = ts - lock->start_ts;
		if (delta < filter.throttle) {
			bpf_map_delete_elem(&start, &pid);
			return 0;
		}

		report.lock = (unsigned long)addr;
		report.pid = pid;
		bpf_get_current_comm(&report.comm, sizeof(report.comm));
		report.start_ts = lock->start_ts;
		report.get_ts = lock->get_ts;
		report.wake_ts = 0;
		report.end_ts = 0;

		report.throttled_time_start = 0;
		report.throttled_time_end = 0;
		report.stack_id = bpf_get_stackid(ctx, &stackmap, KERN_STACKID_FLAGS);

		bpf_map_delete_elem(&start, &pid);
		bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
				&report, sizeof(report));
	} else {
		p = (void *)bpf_get_current_task();
		bpf_probe_read(&sched_task_group, sizeof(sched_task_group), &p->sched_task_group);
		bpf_probe_read(&lock->throttled_time_end, sizeof(u64), &sched_task_group->cfs_bandwidth.throttled_time);
	}

	return 0;
}

static int unlock_exit(u64 addr, enum lock_type type, struct pt_regs *ctx)
{
	u32 pid = bpf_get_current_pid_tgid();
	lock_data *lock;
	int i  = 0;
	u64 throttle_delta, sched_delta;
	u64 delta, ts = bpf_ktime_get_ns();
	lock_report_data report = {};
	struct filter  filter;
	struct arg_info  *argp;

    argp = bpf_map_lookup_elem(&arg_map, &i);
    if (argp)
        filter = _(argp->filter);
    else
        return 0;

	if (filter.addr && filter.addr != (u64)addr)
		return 0;

	lock = bpf_map_lookup_elem(&start, &pid);
	if (!lock)
		return 0;

	delta = ts - lock->start_ts;
	if (delta < filter.throttle) {
		bpf_map_delete_elem(&start, &pid);
		return 0;
	}

	report.lock = addr;
	report.pid = pid;
	bpf_get_current_comm(&report.comm, sizeof(report.comm));
	report.start_ts = lock->start_ts;
	report.get_ts = lock->get_ts;
	report.wake_ts = lock->wake_ts;
	report.end_ts = ts;
	report.switch_cnt = lock->switch_cnt;

	report.throttled_time_start = lock->throttled_time_start;
	report.throttled_time_end = lock->throttled_time_end;
	report.stack_id = bpf_get_stackid(ctx, &stackmap, KERN_STACKID_FLAGS);
	report.cgroup_id = 0;

	if (report.throttled_time_start && report.throttled_time_end)
		throttle_delta = report.throttled_time_end - report.throttled_time_start;

	if (report.wake_ts)
		sched_delta = report.get_ts - report.wake_ts;

	if (throttle_delta >= filter.throttle || sched_delta >= filter.throttle) {
		struct css_set *css_set;
		struct cgroup_subsys_state *cpuacct_subsys;
		struct cgroup *cgroup;
		struct kernfs_node *kn;
		struct task_struct *p;

		p = (void *)bpf_get_current_task();
		bpf_probe_read(&css_set, sizeof(css_set), &p->cgroups);
		bpf_probe_read(&cpuacct_subsys, sizeof(cpuacct_subsys), &css_set->subsys[filter.cpuacct_hierarchy]);
		bpf_probe_read(&cgroup, sizeof(cgroup), &cpuacct_subsys->cgroup);
		bpf_probe_read(&kn, sizeof(kn), &cgroup->kn);
		bpf_probe_read(&report.cgroup_id, sizeof(report.cgroup_id), &kn->id);
	}

	bpf_map_delete_elem(&start, &pid);
	bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
			&report, sizeof(report));

	return 0;
}

SEC("fentry/rwsem_down_write_slowpath")
int BPF_PROG(down_write_entry, struct rw_semaphore *addr)
{
	return lock_entry((u64)addr, LOCK_RWSEM_WRITEELOCK_ENTRY);
}

SEC("fexit/rwsem_down_write_slowpath")
int BPF_PROG(down_write_exit, struct rw_semaphore *addr)
{
	return lock_exit((u64)addr, LOCK_RWSEM_WRITELOCK_EXIT, ctx);
}

SEC("kprobe/up_write")
int up_write(struct pt_regs *ctx)
{
	return unlock_exit((u64)ctx->di, LOCK_RWSEM_WRITEUNLOCK_ENTRY, ctx);
}

SEC("fentry/rwsem_down_read_slowpath")
int BPF_PROG(down_read_entry, struct rw_semaphore *addr)
{
	return lock_entry((u64)addr, LOCK_RWSEM_READELOCK_ENTRY);
}

SEC("fexit/rwsem_down_read_slowpath")
int BPF_PROG(down_read_exit, struct rw_semaphore *addr)
{
	return lock_exit((u64)addr, LOCK_RWSEM_READLOCK_EXIT, ctx);
}

SEC("kprobe/up_read")
int up_read(struct pt_regs *ctx)
{
	return unlock_exit((u64)ctx->di, LOCK_RWSEM_READUNLOCK_ENTRY, ctx);
}

SEC("fentry/__mutex_lock_slowpath")
int BPF_PROG(mutex_lock_entry, struct mutex *addr)
{
	return lock_entry((u64)addr, LOCK_MUTEX_LOCK_ENTRY);
}

SEC("fexit/__mutex_lock_slowpath")
int BPF_PROG(mutex_lock_exit, struct mutex *addr)
{
	return lock_exit((u64)addr, LOCK_MUTEX_LOCK_EXIT, ctx);
}

SEC("kprobe/mutex_unlock")
int mutex_unlock(struct pt_regs *ctx)
{
	return unlock_exit((u64)ctx->di, LOCK_MUTEX_UNLOCK_ENTRY, ctx);
}

SEC("raw_tracepoint/sched_wakeup")
int raw_tracepoint__sched_wakeup(struct bpf_raw_tracepoint_args *ctx)
{
	struct task_struct *p = (void *)ctx->args[0];
	u32 pid;
	struct task_group *sched_task_group;
	lock_data *lock;
	u64 stack_id;

    bpf_probe_read(&pid, sizeof(pid), &p->pid);
	lock = bpf_map_lookup_elem(&start, &pid);

	if (!lock)
		return 0;

	stack_id = bpf_get_stackid(ctx, &stackmap, KERN_STACKID_FLAGS);
	lock->switch_cnt += 1;
	if (lock->get_ts == 0) {
		lock->wake_ts = bpf_ktime_get_ns();
    	bpf_probe_read(&sched_task_group, sizeof(sched_task_group), &p->sched_task_group);
    	bpf_probe_read(&lock->throttled_time_start, sizeof(lock->throttled_time_start), &sched_task_group->cfs_bandwidth.throttled_time);
	}

	return 0;
}

SEC("raw_tracepoint/sched_switch")
int raw_tracepoint__sched_switch(struct bpf_raw_tracepoint_args *ctx)
{
	struct task_struct *p = (void *)ctx->args[1];
	u32 pid;
	int i = 0;
	u64 ts, delta;
	lock_report_data report = {};
	lock_data *lock;
	struct filter  filter;
	struct arg_info  *argp;

   	bpf_probe_read(&pid, sizeof(pid), &p->pid);
	lock = bpf_map_lookup_elem(&start, &pid);
	if (!lock)
		return 0;

    argp = bpf_map_lookup_elem(&arg_map, &i);
    if (argp)
        filter = _(argp->filter);
    else
        return 0;

	ts = bpf_ktime_get_ns();
	delta = ts - lock->start_ts;
	if (delta < filter.throttle)
		return 0;

	lock->switch_cnt += 1;
	bpf_get_current_comm(&report.comm, sizeof(report.comm));
	report.pid = pid;
	report.start_ts = ts;
	report.get_ts = lock->get_ts;
	report.wake_ts = lock->wake_ts;
	report.switch_cnt = lock->switch_cnt;
	report.end_ts = 0;
	report.throttled_time_start = 0;
	report.throttled_time_end = 0;
	report.stack_id = bpf_get_stackid(ctx, &stackmap, KERN_STACKID_FLAGS);
	bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
			&report, sizeof(report));

	return 0;
}

char LICENSE[] SEC("license") = "GPL";

