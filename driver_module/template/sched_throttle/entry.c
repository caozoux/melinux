#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/kernfs.h>
#include <linux/cgroup-defs.h>

#include <kernel/sched/sched.h>
#include <kernel/cgroup/cgroup-internal.h>

static inline struct task_struct *st_task_of(struct sched_entity *se)
{
	return container_of(se, struct task_struct, se);
}

static inline struct task_group *st_css_tg(struct cgroup_subsys_state *css)
{
	return css ? container_of(css, struct task_group, css) : NULL;
}

#define ECHO(fmt, ...) \
	trace_printk("KHACK: "fmt, ##__VA_ARGS__)

#define ERR(fmt, ...) \
	printk("%s KHACK: "fmt, __func__, ##__VA_ARGS__)

#define SCHED_TRACE_TASK_COMM_LEN 32
#define SCHED_TRACE_TASKS_NUM 32

enum sched_evt_type {
	SCHED_EVT_RUN,
	SCHED_EVT_THROT,
	SCHED_EVT_MIGR,
	SCHED_EVT_WAKE,
	SCHED_EVT_ENQUEUE,
	SCHED_EVT_MAX,
};

enum sched_evt_run_state {
	SCHED_EVT_RUN_RUNNING,
	SCHED_EVT_RUN_SLEEP,
	SCHED_EVT_RUN_DISK,
	SCHED_EVT_RUN_DEAD,
	SCHED_EVT_RUN_OTHER,
	SCHED_EVT_RUN_MAX,
};

static char sched_evt_run_char[SCHED_EVT_RUN_MAX] = {
	'R', 'S', 'D', 'K', 'O'
};

enum sched_stat_time {
	SCHED_STATE_RUN,
	SCHED_STATE_USER,
	SCHED_STATE_SYS,
	SCHED_STATE_HARDIRQ,
	SCHED_STATE_SOFTIRQ,
	SCHED_STATE_SLEEP,
	SCHED_STATE_BLOCK,
	SCHED_STATE_DELAY,
	SCHED_STATE_MAX,
};

struct sched_task {
	int pid;
	u8 state;
	u64 time;
	u64 sys;
	u64 user;
	u64 hardirq;
	u64 softirq;
	char comm[SCHED_TRACE_TASK_COMM_LEN];
};

struct sched_throt {
	int cgrp_id;
	u64 time;
};

struct sched_evt {
	enum sched_evt_type type;
	u64 time;
	union {
		struct sched_task tsk;
		struct sched_throt throt;
	};
};

struct sched_trace_rq {
	bool start;
	int cpu;
	struct rq *rq;

	struct {
		struct task_struct *tsk;
		u64 exec_start;
		u64 sys;
		u64 user;
		u64 hardirq;
		u64 softirq;
	} curr;

	struct {
		int head;
		int tail;
		struct sched_evt evts[SCHED_TRACE_TASKS_NUM];
	} activity;
};

enum st_mode {
	KHACK_MODE_NOP = 0,
	KHACK_MODE_SCHED_LAT,
	KHACK_MODE_MAX
};

const char *st_mode_str[] = {
	"nop",
	"sched_lat",
};

const char *trace_mode_str[] = {
	"nop",
	"sched_lat",
};

struct sched_trace_config {
	enum st_mode mode;
	struct {
		struct task_group *tg;
		int lat;
	} sched;
};

DEFINE_PER_CPU(struct sched_trace_rq, st_rqs);

struct sched_trace_config KCONF;

static void (*update_rq_clock_shawdow)(struct rq *rq);
static struct static_key_false *sched_schedstats_shadow;
static struct rq __percpu *runqueues_shadow;
static struct task_group *root_task_group_shadow;
static struct cgroup_subsys **cgroup_subsys_shadow;

static struct cgroup_subsys_state *st_cgroup_css(struct cgroup *cgrp,
					      struct cgroup_subsys *ss)
{
	if (ss)
		return rcu_dereference_check(cgrp->subsys[ss->id],
					lockdep_is_held(&cgroup_mutex));
	else
		return &cgrp->self;
}

struct cgroup_subsys_state *global_cgroup_css(struct cgroup *cgrp,
						int ssid)
{
	return st_cgroup_css(cgrp, cgroup_subsys_shadow[(ssid)]);
}

static bool inline task_need_trace(struct task_struct *tsk)
{
	if (is_idle_task(tsk))
		return false;

	return !KCONF.sched.tg || (KCONF.sched.tg == tsk->sched_task_group);
}

/*
 *       tail       head
 *          v       v
 *  |-------xxxxxxxx--------------|
 *
 */
static void sched_log_push(struct sched_trace_rq *krq)
{
	int head = krq->activity.head;
	int tail = krq->activity.tail;

	head = (head + 1) % SCHED_TRACE_TASKS_NUM;

	if ((head + 1) % SCHED_TRACE_TASKS_NUM == tail)
		krq->activity.tail = (tail + 1) % SCHED_TRACE_TASKS_NUM;

	krq->activity.head = head;
}

static void sched_trace_lat_reset(void)
{
	struct sched_trace_rq *krq;
	int cpu;

	for_each_possible_cpu(cpu) {
		krq = per_cpu_ptr(&st_rqs, cpu);
		krq->activity.head = 0;
		krq->activity.tail = 0;
		krq->curr.tsk = NULL;
	}
}

static bool sched_check_ancestor(struct task_struct *tsk, int cgrp_id)
{
	struct task_group *tg = tsk->se.cfs_rq->tg;
	bool yes = false;

	while (tg && tg != root_task_group_shadow) {
		if (tg->css.id == cgrp_id) {
			yes = true;
			break;
		}
		tg = tg->parent;

	}
	return yes;
}

static void sched_log_echo(struct sched_trace_rq *krq,
		struct task_struct *tsk)
{
	struct sched_evt *evt;
	bool stop = false;
	int i;

	/*
	 * Log is empty, just quit
	 */
	if (krq->activity.head == krq->activity.tail)
	    return;

	ECHO("Sched Activity of CPU %d:\n", krq->cpu);
	i = krq->activity.head ? (krq->activity.head - 1) :
	                         (SCHED_TRACE_TASKS_NUM - 1);
	while (1) {
		evt = &krq->activity.evts[i];
		switch (evt->type) {
		case SCHED_EVT_RUN:
			ECHO("SCHED: %s:%d state %c %lld(%lld %lld %lld %lld) us\n",
					evt->tsk.comm, evt->tsk.pid,
					sched_evt_run_char[evt->tsk.state],
					evt->tsk.time >> 10,
					evt->tsk.user >> 10,
					evt->tsk.sys >> 10,
					evt->tsk.hardirq >> 10,
					evt->tsk.softirq >> 10);
			break;
		case SCHED_EVT_THROT:
			ECHO("THROT: cgroup %d %s throttled %lld us\n",
					evt->throt.cgrp_id,
					sched_check_ancestor(tsk, evt->throt.cgrp_id) ? "ancestor" : "unknown",
					evt->throt.time >> 10);
			break;
		case SCHED_EVT_MIGR:
			ECHO("MIGR: %s:%d delay %lld us\n", evt->tsk.comm, evt->tsk.pid,
					evt->tsk.time >> 10);
			break;
		case SCHED_EVT_WAKE:
			ECHO("WAKE: %s:%d\n", evt->tsk.comm, evt->tsk.pid);
			break;
		case SCHED_EVT_ENQUEUE:
			ECHO("ENQUEUE: %s:%d\n", evt->tsk.comm, evt->tsk.pid);
			break;
		default:
			break;
		}

		switch (evt->type) {
		case SCHED_EVT_RUN:
		case SCHED_EVT_WAKE:
		case SCHED_EVT_MIGR:
		case SCHED_EVT_ENQUEUE:
			if (evt->tsk.pid == tsk->pid)
				stop = true;
			break;
		default:
			break;
		}

		if (i == krq->activity.tail || stop)
			break;

		if (i)
			i--;
		else
			i = SCHED_TRACE_TASKS_NUM - 1;
	}
}

static void sched_log_run(struct sched_trace_rq *krq,
		struct rq *rq, struct task_struct *tsk)
{
	struct kernel_cpustat *kstat = this_cpu_ptr(&kernel_cpustat);
	struct sched_evt *evt;
	struct sched_task *ktsk;

	evt = &krq->activity.evts[krq->activity.head];
	evt->type = SCHED_EVT_RUN;

	ktsk = &evt->tsk;
	snprintf(ktsk->comm, SCHED_TRACE_TASK_COMM_LEN, "%s", tsk->comm);
	ktsk->pid = tsk->pid;

	switch (tsk->__state) {
	case TASK_RUNNING:
		ktsk->state = SCHED_EVT_RUN_RUNNING;
		break;
	case TASK_INTERRUPTIBLE:
		ktsk->state = SCHED_EVT_RUN_SLEEP;
		break;
	case TASK_UNINTERRUPTIBLE:
		ktsk->state = SCHED_EVT_RUN_DISK;
		break;
	case TASK_DEAD:
		ktsk->state = SCHED_EVT_RUN_DEAD;
		break;
	default:
		ktsk->state = 0;
		break;
	}
	/*
	 * Use rq_clock() instead of rq_clock_task() because wait_start
	 * use it.
 	 */
	evt->time = rq_clock(rq);
	ktsk->time = evt->time - krq->curr.exec_start;

	ktsk->user = kstat->cpustat[CPUTIME_USER] - krq->curr.user;
	ktsk->sys = kstat->cpustat[CPUTIME_SYSTEM] - krq->curr.sys;
	ktsk->hardirq = kstat->cpustat[CPUTIME_IRQ] - krq->curr.hardirq;
	ktsk->softirq = kstat->cpustat[CPUTIME_SOFTIRQ] - krq->curr.softirq;

	sched_log_push(krq);
}

/*
 * context_switch is a better place to trace, but it is always inlined
 * pick_next_task is also optimized in non-debug kernel
 */
static void sched_lat_finish_task_switch(struct pt_regs *regs)
{
	/*
	 * finish_task_switch() must be run on local cpu, this_cpu_ptr is OK
	 */
	struct sched_trace_rq *krq = this_cpu_ptr(&st_rqs);
	struct rq *rq = this_cpu_ptr(runqueues_shadow);
	struct task_struct *nxt = current;
	struct task_struct *prev = (void *)regs->di;

	if (nxt == prev)
		return;

	/*
	 * Don't use lockdep_assert_rq_held(rq) here as the lockdep has been
 	 * released by prepare_lock_switch()
 	 */
	update_rq_clock_shawdow(rq);

	if (!is_idle_task(prev))
		sched_log_run(krq, rq, prev);

	if (is_idle_task(nxt)) {
		krq->curr.tsk = NULL;
	} else {
		struct kernel_cpustat *kstat = this_cpu_ptr(&kernel_cpustat);

		krq->curr.tsk = nxt;
		krq->curr.exec_start = rq_clock(rq);
		krq->curr.user = kstat->cpustat[CPUTIME_USER];
		krq->curr.sys = kstat->cpustat[CPUTIME_SYSTEM];
		krq->curr.hardirq = kstat->cpustat[CPUTIME_IRQ];
		krq->curr.softirq = kstat->cpustat[CPUTIME_SOFTIRQ];
	}
}

static int pre_finish_task_switch(struct kprobe *unused,
				    struct pt_regs *regs)
{
	struct sched_trace_rq *krq = this_cpu_ptr(&st_rqs);

	if (!krq->start && is_idle_task(current)) {
		krq->start = true;
		return 0;
	}

	switch (KCONF.mode) {
	case KHACK_MODE_SCHED_LAT:
		sched_lat_finish_task_switch(regs);
		break;
	default:
		break;
	}
	return 0;
}

static struct kprobe kp_finish_task_switch = {
	.symbol_name = "finish_task_switch",
	.pre_handler = pre_finish_task_switch,
};

static void sched_lat_check(struct task_struct *tsk, u64 lat)
{
	struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, task_cpu(tsk));

	lat = lat >> 10;
	if (lat < KCONF.sched.lat)
		return;

	ECHO("%s:%d:%c sched lat %lld us is too long\n",
			tsk->comm, tsk->pid,
			task_on_rq_migrating(tsk) ? 'M' : 'S',
			lat);

	if (krq->curr.tsk) {
		struct kernel_cpustat *kstat = per_cpu_ptr(&kernel_cpustat, task_cpu(tsk));

		ECHO("current of cpu %d %s:%d runs %lld us (%lld %lld %lld %lld)\n",
				krq->cpu, krq->curr.tsk->comm, krq->curr.tsk->pid,
				(rq_clock(krq->rq) - krq->curr.exec_start) >> 10,
				(kstat->cpustat[CPUTIME_USER] - krq->curr.user) >> 10,
				(kstat->cpustat[CPUTIME_SYSTEM] - krq->curr.sys) >> 10,
				(kstat->cpustat[CPUTIME_IRQ] - krq->curr.hardirq) >> 10,
				(kstat->cpustat[CPUTIME_SOFTIRQ] - krq->curr.softirq) >> 10);
	}

	sched_log_echo(krq, tsk);
}

/*
 * the pick_next_task() has not returned, so the SCHED_EVT_RUN log
 * has not been there, we also need to check the krq->curr as the
 * dequeue_entity() one.
 */
static void sched_lat_dequeue_entity(struct pt_regs *regs)
{
	struct sched_entity *se = (void *)regs->si;
	struct cfs_rq *cfs_rq = (void *)regs->di;
	struct task_struct *tsk;

	if (!entity_is_task(se) || se == cfs_rq->curr)
		return;

	tsk = st_task_of(se);
	if (!task_on_rq_migrating(tsk))
		return;

	if (task_need_trace(tsk)) {
		u64 lat = rq_clock(cfs_rq->rq) - tsk->stats.wait_start;
		sched_lat_check(tsk, lat);
	}
}

static int pre_dequeue_entity(
		struct kprobe *unused, struct pt_regs *regs)
{
	struct cfs_rq *cfs_rq = (void *)regs->di;
	struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, cpu_of(cfs_rq->rq));

	if (!krq->start)
		return 0;

	switch (KCONF.mode) {
	case KHACK_MODE_SCHED_LAT:
		sched_lat_dequeue_entity(regs);
		break;
	default:
		break;
	}

	return 0;
}

static struct kprobe kp_dequeue_entity = {
	.symbol_name = "dequeue_entity",
	.pre_handler = pre_dequeue_entity,
};

static void sched_lat_enqueue_task_fair(struct pt_regs *regs)
{
	struct task_struct *tsk = (void *)regs->si;
	struct rq *rq = (void *)regs->di;
	struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, rq->cpu);
	struct sched_evt *evt = &krq->activity.evts[krq->activity.head];

	if (task_on_rq_migrating(tsk)) {
		evt->type = SCHED_EVT_MIGR;
		/*
		 * When dequeue, the task's wait_start has been converted to
		 * a relatively value, we can use it directly. Refer to
 		 * update_stats_wait_end()
 		 */
		evt->tsk.time = tsk->stats.wait_start;
	} else if (!task_on_rq_queued(tsk))
		evt->type = SCHED_EVT_WAKE;
	else
		evt->type = SCHED_EVT_ENQUEUE; /* wake_up_new_task is such a case */

	snprintf(evt->tsk.comm, SCHED_TRACE_TASK_COMM_LEN, "%s", tsk->comm);
	evt->tsk.pid = tsk->pid;

	evt->time = rq_clock(rq);

	sched_log_push(krq);
}

static int pre_enqueue_task_fair(
		struct kprobe *unused, struct pt_regs *regs)
{
	struct rq *rq = (void *)regs->di;
	struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, cpu_of(rq));

	if (!krq->start)
		return 0;

	switch (KCONF.mode) {
	case KHACK_MODE_SCHED_LAT:
		sched_lat_enqueue_task_fair(regs);
		break;
	default:
		break;
	}

	return 0;
}

static struct kprobe kp_enqueue_task_fair = {
	.symbol_name = "enqueue_task_fair",
	.pre_handler = pre_enqueue_task_fair,
};

static void sched_lat_unthrottle_cfs_rq(struct pt_regs *regs)
{
	struct cfs_rq *cfs_rq = (void *)regs->di;
	struct rq *rq = cfs_rq->rq;
	struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, rq->cpu);
	struct sched_evt *evt;
	u64 now;

	evt = &krq->activity.evts[krq->activity.head];
	evt->type = SCHED_EVT_THROT;

	update_rq_clock_shawdow(rq);

	now = rq_clock(rq);

	evt->time = now;
	evt->throt.cgrp_id = cfs_rq->tg->css.id;
	evt->throt.time = now - cfs_rq->throttled_clock;

	sched_log_push(krq);
}

static int pre_unthrottle_cfs_rq(struct kprobe *unused,
				    struct pt_regs *regs)
{
	struct cfs_rq *cfs_rq = (void *)regs->di;
	struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, cpu_of(cfs_rq->rq));

	if (!krq->start)
		return 0;

	switch (KCONF.mode) {
	case KHACK_MODE_SCHED_LAT:
		sched_lat_unthrottle_cfs_rq(regs);
		break;
	default:
		break;
	}

	return 0;
}

static struct kprobe kp_untrottle_cfs_rq = {
	.symbol_name = "unthrottle_cfs_rq",
	.pre_handler = pre_unthrottle_cfs_rq,
};

static void sched_lat_set_next_entity(struct pt_regs *regs)
{
	struct sched_entity *se = (void *)regs->si;

	if (entity_is_task(se) && task_need_trace(st_task_of(se))) {
		struct cfs_rq *cfs_rq = (void *)regs->di;
		sched_lat_check(st_task_of(se),
			rq_clock(cfs_rq->rq) - st_task_of(se)->stats.wait_start);
	}
}

static int pre_set_next_entity(struct kprobe *unused,
				    struct pt_regs *regs)
{
	struct cfs_rq *cfs_rq = (void *)regs->di;
	struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, cpu_of(cfs_rq->rq));

	if (!krq->start)
		return 0;

	switch (KCONF.mode) {
	case KHACK_MODE_SCHED_LAT:
		sched_lat_set_next_entity(regs);
		break;
	default:
		break;
	}

	return 0;
}

static struct kprobe kp_set_next_entity = {
	.symbol_name = "set_next_entity",
	.pre_handler = pre_set_next_entity,
};

static struct kprobe *KPS[] = {
	&kp_finish_task_switch,
	&kp_dequeue_entity,
	&kp_enqueue_task_fair,
	&kp_untrottle_cfs_rq,
	&kp_set_next_entity,
};

static int parse_task_group(char *dir, bool top)
{
	struct kernfs_open_file *of;
	struct cgroup *cgrp;
	struct cgroup_subsys_state *css;
	struct task_group *tg;
	struct file *filp;
	int err;
	/*
	 * The cgroup directory is really long sometimes
	 */
	static char buf[512];

	if (!strcmp(dir, "all")) {
		err = top ? -EINVAL : 0;
		KCONF.sched.tg = NULL;
		return err;
	}

	snprintf(buf, 512, "/sys/fs/cgroup/cpu,cpuacct/%s/cgroup.procs", dir);
	filp = filp_open(buf, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		ERR("open %s failed %ld\n", buf, PTR_ERR(filp));
		return PTR_ERR(filp);;
	}
	/*
	 * This is copied from kernfs_of()
	 */
	of = ((struct seq_file *)filp->private_data)->private;
	cgrp = of->kn->parent->priv;

	rcu_read_lock();
	css = global_cgroup_css(cgrp, cpu_cgrp_id);
	tg = st_css_tg(css);
	if (top && tg->parent != root_task_group_shadow)
		err = -EINVAL;
	else
		err = 0;
	rcu_read_unlock();

	filp_close(filp, NULL);

	KCONF.sched.tg = tg;

	return err;
}

static void trace_nop(void)
{
	int old = KCONF.mode;

	KCONF.mode = KHACK_MODE_NOP;
	/*
	 * Guarantee all of the cpu has see the new mode
	 */
	synchronize_rcu();

	switch (old) {
	case KHACK_MODE_SCHED_LAT:
		sched_trace_lat_reset();
		break;
	default:
		break;
	}
}

static int sched_trace_lat_set(const char *val,
	const struct kernel_param *kp)
{
	char *buf, *pos, *nxt;
	int err, msec;

	buf = kstrdup(val, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	nxt = buf;
	pos = strsep(&nxt, " \t\n");
	if (!strcmp(pos, "nop")) {
		trace_nop();
	} else if (!strcmp(pos, "sched-latency")) {
		pos = strsep(&nxt, " \t\n");
		err = parse_task_group(pos, false);
		if (err)
			goto out;
	
		pos = strsep(&nxt, " \t\n");
		err = kstrtou32(pos, 10, &msec);
		if (err)
			goto out;
	
		KCONF.sched.lat = msec << 10;

		/*
  		 * Switch to nop mode first to reset the previous state
 		 */
		trace_nop();
		KCONF.mode = KHACK_MODE_SCHED_LAT;
	}else {
		ERR("Sched Trace Command Line Usage:\n");
		ERR("sched-latency <cgroup name> <latency>, trace\n");
	}

	err = 0;
out:
	kfree(buf);
	return err;
}


static int sched_trace_mode_get(char *buf, const struct kernel_param *kp)
{
	int cnt;

	cnt = sprintf(buf, "KHACK %s mode:\n", trace_mode_str[KCONF.mode]);

	return cnt;
}

static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};

static bool __init sched_trace_syms(void)
{
	const char *name;
	typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
	kallsyms_lookup_name_t kallsyms_lookup_name;

	register_kprobe(&kp);
	kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
	unregister_kprobe(&kp);

	name = "update_rq_clock";
	update_rq_clock_shawdow = (void *)kallsyms_lookup_name(name);
	if (!update_rq_clock_shawdow)
		goto fail;

	name = "sched_schedstats";
	sched_schedstats_shadow = (void *)kallsyms_lookup_name(name);
	if (!sched_schedstats_shadow)
		goto fail;

	name = "runqueues";
	runqueues_shadow = (void *)kallsyms_lookup_name(name);
	if (!runqueues_shadow)
		goto fail;

	name = "root_task_group";
	root_task_group_shadow = (void *)kallsyms_lookup_name(name);
	if (!root_task_group_shadow)
		goto fail;

	name = "cgroup_subsys";
	cgroup_subsys_shadow = (void *)kallsyms_lookup_name(name);
	if (!cgroup_subsys_shadow)
		goto fail;

	return true;
fail:
	ERR("cannot lookup sym of %s\n", name);
	return false;
}

static bool __init schedstats_check(void)
{
	if (!atomic_read(&sched_schedstats_shadow->key.enabled)) {
		ERR("Please enable /proc/sys/kernel/sched_schedstats\n");
		return false;
	}

	return true;
}

static void khack_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		struct sched_trace_rq *krq = per_cpu_ptr(&st_rqs, cpu);
		struct rq *rq = per_cpu_ptr(runqueues_shadow, cpu);

		krq->cpu = cpu;
		krq->rq = rq;
	}

	KCONF.mode = KHACK_MODE_NOP;
	KCONF.sched.tg = NULL;
	KCONF.sched.lat = 2000;
}

static int __init sched_trace_enter(void)
{
	int i, nr, ret;

	if (!sched_trace_syms() || !schedstats_check())
		return -EINVAL;

	khack_init();

	nr = sizeof(KPS)/sizeof(void *);
	for (i = 0; i < nr; i++) {
		ret = register_kprobe(KPS[i]);
		if (ret) {
			ERR("register kprobe for %s failed %d\n",
					KPS[i]->symbol_name, ret);
			if (i > 0)
				unregister_kprobes(KPS, i);
			break;
		}
	}

	return ret;
}

static void __exit sched_trace_exit(void)
{
	unregister_kprobes(KPS, sizeof(KPS)/sizeof(void *));
}

module_init(sched_trace_enter);
module_exit(sched_trace_exit);

module_param_call(latency, sched_trace_lat_set,
	sched_trace_mode_get, NULL, S_IWUSR | S_IRUSR);

MODULE_AUTHOR("LiuHua");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("kwai schedule trace module");
