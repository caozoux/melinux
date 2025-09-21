#include <argp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <pthread.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "lockdetect.h"
#include "./bpf/lockdetect.skel.h"

#define MAX_FIFO_SIZE (30)
#define SOFT_THROTTLE ((3UL)*60*1000000000)
#define MULT_CPU_MAX  (5)

#define MAX_LINE_LENGTH 1024
#define MAX_TOKENS (1024*5)

struct cgroup_patch {
	int cgroup_id;
	char path[1024];
};


static int cpuacct_hierarchy = 0;
static struct cgroup_patch cgroup_path_list[MAX_TOKENS];
static int cgroup_patch_index = 0;
static int cgroup_path_show = 0;

struct arguments
{
    int verbose;
    char *mode;
    int throttle_time;
    int depth;
};

static int fault_fd;
struct env {
	time_t duration;
	bool verbose;
	struct filter filter;
} env;

volatile sig_atomic_t exiting = 0;

static enum LOAD_MODE load_mode = LOAD_NONE;

static int stackmp_fd;
static struct ksym *ksyms;
static bool debug_en;
const char *argp_program_version = "lockdetect 0.1";
const char argp_program_doc[] =
"detect the rwsem/mutex lock latency\n"
"\n"
"USAGE: lockdtect [--help] [-f LOGFILE]\n"
"\n"
"EXAMPLES:\n"
"    lockdetect            # detect lock latency\n"
"    lockdetect -d         # Verbose debug output\n";

static const struct argp_option opts[] = {
	{ "debug", 'd', NULL, 0, "Verbose debug output"},
	{ "throttle", 't', "throttle_time",  0, "lock throttle time(ms)"},
	{ "mode", 'm', "mode", 0, "rwsem/rwsem_run/rwsem_run_switch/mutex/mutex_run/mutex_run_switch"},
	{ "address", 'a', "address", 0, "specift lock address"},
	{ "cgroup", 'c', NULL, 0, "show cgroup path"},
	{ NULL, 'h', NULL, OPTION_HIDDEN, "Show the full help"},
	{},
};

int unix_socket_fd;
pthread_t report_th;

int getMachineCgroupPath(void)
{
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char *token;

    cgroup_patch_index= 0;
    fp = popen("cat /proc/cgroups  | grep -n cpuacct", "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    if (fgets(line, sizeof(line), fp) != NULL) {
        token = strtok(line, ":");
		if (!token) {
			perror("/proc/groups no cpuacct\n");
			exit(EXIT_FAILURE);
		}
		cpuacct_hierarchy = atoi(token) - 2;
		if (cpuacct_hierarchy < 0) {
			perror("get /proc/groups cpuacct index failed\n");
			exit(EXIT_FAILURE);
		}
	} else {
        perror("get cpuacct hierarchy failed");
        exit(EXIT_FAILURE);
	}
    if (pclose(fp) == -1) {
        perror("pclose");
        exit(EXIT_FAILURE);
    }

	memset(cgroup_path_list, 0, sizeof(cgroup_path_list));
    // Execute the `find` command and open a pipe to read its output
    fp = popen("find /sys/fs/cgroup/cpu,cpuacct/ -type d -exec /bin/ls -id {} \\;", "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    // Read each line from the output
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove the newline character at the end, if present

        line[strcspn(line, "\n")] = '\0';

        // Tokenize the line using space as the delimiter
        token = strtok(line, " ");

		cgroup_path_list[cgroup_patch_index].cgroup_id = atoi(token);
        token = strtok(NULL, " ");
		memcpy((void*)cgroup_path_list[cgroup_patch_index].path, token, 1024);

		printf("%s %d\n", cgroup_path_list[cgroup_patch_index].path, cgroup_path_list[cgroup_patch_index].cgroup_id);
		cgroup_patch_index++;
        if (cgroup_patch_index > MAX_TOKENS)
			break;
    }

    // Close the pipe
    if (pclose(fp) == -1) {
        perror("pclose");
        exit(EXIT_FAILURE);
    }

    return 0;
}

void show_cpuacct_path(int cgroup_id)
{
	int i;

	for (i = 0; i < cgroup_patch_index; i++) {
		if (cgroup_path_list[i].cgroup_id == cgroup_id) {
			printf(" %s", cgroup_path_list[i].path);
			break;
		}
	}

	if (i == cgroup_patch_index)
			printf(" NULL");
}

static error_t parse_arg(int key, char *arg, struct argp_state *state)
{
	int ret = errno;
	__u64 addr;
	static int pos_args;

	switch (key) {
	case 'h':
		argp_state_help(state, stderr, ARGP_HELP_STD_HELP);
		break;
	case 'd':
		env.verbose = true;
		debug_en = true;
		break;
	case 'a':
		addr = strtoull(arg, NULL, 16);
		env.filter.addr = addr;
		break;
	case 't':
		env.filter.throttle = strtol(arg, NULL, 10);
		env.filter.throttle *= 1000000;
		break;
	case 'c':
		getMachineCgroupPath();
		cgroup_path_show = 1;
		break;
	case 'm':
		if (!strcmp(arg, "rwsem"))
			load_mode = LOAD_RWSEM;
		else if (!strcmp(arg, "rwsem_run"))
			load_mode = LOAD_RWSEM_RUN;
		else if (!strcmp(arg, "rwsem_run_switch"))
			load_mode = LOAD_RWSEM_RUN_SWITCH;
		else if (!strcmp(arg, "mutex"))
			load_mode = LOAD_MUTEX;
		else if (!strcmp(arg, "mutex_run"))
			load_mode = LOAD_MUTEX_RUN;
		else if (!strcmp(arg, "mutex_run_switch"))
			load_mode = LOAD_MUTEX_RUN_SWITCH;
		else {
			fprintf(stderr,"mode %s not support\n", arg);
			return -1;
		}
		break;
	case ARGP_KEY_ARG:
		if (pos_args++) {
			fprintf(stderr,
				"unrecognized positional argument: %s\n", arg);
			argp_usage(state);
		}
		errno = 0;
		env.duration = strtol(arg, NULL, 10);
		if (errno) {
			ret = errno;
			fprintf(stderr, "invalid duration\n");
			argp_usage(state);
			return ret;
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static void bump_memlock_rlimit(void)
{
	struct rlimit rlim_new = {
		.rlim_cur = RLIM_INFINITY,
		.rlim_max = RLIM_INFINITY,
	};

	if (setrlimit(RLIMIT_MEMLOCK, &rlim_new)) {
		fprintf(stderr, "Failed to increase RLIMIT_MEMLOCK limit!\n");
		exit(1);
	}
}

void lock_event(void *ctx, int cpu, void *data, __u32 data_sz)
{
	const lock_report_data *e = data;
	unsigned long wait_ts = 0, run_ts = 0;
	unsigned long sched_ts = 0, throttle_ts = 0;
	struct tm *tm_info;
	time_t t;
	char buffer[26];

	//some time get_ts or start_ts is lost, ignore it
	if (!e->get_ts || !e->start_ts)
		return;

	time(&t);
	tm_info = localtime(&t);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
	if (load_mode == LOAD_RWSEM || load_mode == LOAD_MUTEX) {
		wait_ts = e->get_ts - e->start_ts;
		run_ts = 0;
		sched_ts = 0;
		throttle_ts = 0;
		printf("%s %s;%d;0x%lx;%ld;%ld;%ld;%ld;", buffer, e->comm, e->pid, e->lock,
				wait_ts, run_ts, sched_ts, throttle_ts);
		print_stack(stackmp_fd, e->stack_id, ksyms);
		printf("\n");
	} else if (load_mode == LOAD_RWSEM_RUN || load_mode == LOAD_MUTEX_RUN) {
		wait_ts = e->get_ts - e->start_ts;
		run_ts = e->end_ts - e->get_ts;
		sched_ts = e->wake_ts ? e->get_ts - e->wake_ts :0;
		if (!e->throttled_time_end || !e->throttled_time_start)
			throttle_ts = 0;
		else
			throttle_ts = e->throttled_time_end - e->throttled_time_start;

		if (debug_en || run_ts >= env.filter.throttle || throttle_ts >= env.filter.throttle
				|| sched_ts >= env.filter.throttle) {
			printf("%s %s;%d;0x%lx;%ld;%ld;%ld;%ld;%d;%d;", buffer, e->comm, e->pid, e->lock,
					wait_ts, run_ts, sched_ts, throttle_ts, e->switch_cnt, e->cgroup_id);
			print_stack(stackmp_fd, e->stack_id, ksyms);
			if (e->cgroup_id && cgroup_path_show)
				show_cpuacct_path(e->cgroup_id);
			printf("\n");
		}
	} else if (load_mode == LOAD_RWSEM_RUN_SWITCH || load_mode == LOAD_MUTEX_RUN_SWITCH) {
		wait_ts = e->get_ts - e->start_ts;
		run_ts = e->end_ts - e->get_ts;
		sched_ts = e->wake_ts ? e->get_ts - e->wake_ts :0;
		if (!e->throttled_time_end || !e->throttled_time_start)
			throttle_ts = 0;
		else
			throttle_ts = e->throttled_time_end - e->throttled_time_start;

		if (e->end_ts == 0) {
			printf("switch_stack:%s;%d;0x%lx;%ld;%d;", e->comm, e->pid, e->lock
					 , e->start_ts, e->switch_cnt);
			print_stack(stackmp_fd, e->stack_id, ksyms);
			printf("\n");
		} else if (debug_en || run_ts >= env.filter.throttle || throttle_ts >= env.filter.throttle
				|| sched_ts >= env.filter.throttle) {
			printf("%s %s;%d;0x%lx;%ld;%ld;%ld;%ld;%d;", buffer, e->comm, e->pid, e->lock
					, wait_ts, run_ts, sched_ts, throttle_ts, e->switch_cnt);
			print_stack(stackmp_fd, e->stack_id, ksyms);
			printf("\n");
		}
	}

	return;
}

static int ebpf_dynamic_attach(struct lockdetect_bpf *obj, enum LOAD_MODE mode)
{
	struct bpf_object_skeleton *s = obj->skeleton;
	int ret = 0;

	switch (mode) {
		case LOAD_RWSEM:
			s->prog_cnt = 4;

			s->progs[0].name = "down_write_entry";
			s->progs[0].prog = &obj->progs.down_write_entry;
			s->progs[0].link = &obj->links.down_write_entry;

			s->progs[1].name = "down_write_exit";
			s->progs[1].prog = &obj->progs.down_write_exit;
			s->progs[1].link = &obj->links.down_write_exit;

			s->progs[2].name = "down_write_entry";
			s->progs[2].prog = &obj->progs.down_read_entry;
			s->progs[2].link = &obj->links.down_read_entry;

			s->progs[3].name = "down_write_exit";
			s->progs[3].prog = &obj->progs.down_read_exit;
			s->progs[3].link = &obj->links.down_read_exit;
			break;

		case LOAD_RWSEM_RUN:
			s->prog_cnt = 7;
			s->progs[0].name = "down_write_entry";
			s->progs[0].prog = &obj->progs.down_write_entry;
			s->progs[0].link = &obj->links.down_write_entry;

			s->progs[1].name = "down_write_exit";
			s->progs[1].prog = &obj->progs.down_write_exit;
			s->progs[1].link = &obj->links.down_write_exit;

			s->progs[2].name = "up_write";
			s->progs[2].prog = &obj->progs.up_write;
			s->progs[2].link = &obj->links.up_write;

			s->progs[3].name = "down_read_entry";
			s->progs[3].prog = &obj->progs.down_read_entry;
			s->progs[3].link = &obj->links.down_read_entry;

			s->progs[4].name = "down_read_exit";
			s->progs[4].prog = &obj->progs.down_read_exit;
			s->progs[4].link = &obj->links.down_read_exit;

			s->progs[5].name = "up_read";
			s->progs[5].prog = &obj->progs.up_read;
			s->progs[5].link = &obj->links.up_read;

			s->progs[6].name = "raw_tracepoint__sched_wakeup";
			s->progs[6].prog = &obj->progs.raw_tracepoint__sched_wakeup;
			s->progs[6].link = &obj->links.raw_tracepoint__sched_wakeup;

			break;

		case LOAD_RWSEM_RUN_SWITCH:
			s->prog_cnt = 7;
			s->progs[0].name = "down_write_entry";
			s->progs[0].prog = &obj->progs.down_write_entry;
			s->progs[0].link = &obj->links.down_write_entry;

			s->progs[1].name = "down_write_exit";
			s->progs[1].prog = &obj->progs.down_write_exit;
			s->progs[1].link = &obj->links.down_write_exit;

			s->progs[2].name = "up_write";
			s->progs[2].prog = &obj->progs.up_write;
			s->progs[2].link = &obj->links.up_write;

			s->progs[3].name = "down_read_entry";
			s->progs[3].prog = &obj->progs.down_read_entry;
			s->progs[3].link = &obj->links.down_read_entry;

			s->progs[4].name = "down_read_exit";
			s->progs[4].prog = &obj->progs.down_read_exit;
			s->progs[4].link = &obj->links.down_read_exit;

			s->progs[5].name = "up_read";
			s->progs[5].prog = &obj->progs.up_read;
			s->progs[5].link = &obj->links.up_read;

			s->progs[6].name = "raw_tracepoint__sched_switch";
			s->progs[6].prog = &obj->progs.raw_tracepoint__sched_switch;
			s->progs[6].link = &obj->links.raw_tracepoint__sched_switch;

			break;

		case LOAD_MUTEX:
			s->prog_cnt = 2;

			s->progs[0].name = "mutex_lock_entry";
			s->progs[0].prog = &obj->progs.mutex_lock_entry;
			s->progs[0].link = &obj->links.mutex_lock_entry;

			s->progs[1].name = "mutex_lock_exit";
			s->progs[1].prog = &obj->progs.mutex_lock_exit;
			s->progs[1].link = &obj->links.mutex_lock_exit;

			break;

		case LOAD_MUTEX_RUN:
			s->prog_cnt = 4;

			s->progs[0].name = "mutex_lock_entry";
			s->progs[0].prog = &obj->progs.mutex_lock_entry;
			s->progs[0].link = &obj->links.mutex_lock_entry;

			s->progs[1].name = "mutex_lock_exit";
			s->progs[1].prog = &obj->progs.mutex_lock_exit;
			s->progs[1].link = &obj->links.mutex_lock_exit;

			s->progs[2].name = "mutex_unlock";
			s->progs[2].prog = &obj->progs.mutex_unlock;
			s->progs[2].link = &obj->links.mutex_unlock;

			s->progs[3].name = "raw_tracepoint__sched_wakeup";
			s->progs[3].prog = &obj->progs.raw_tracepoint__sched_wakeup;
			s->progs[3].link = &obj->links.raw_tracepoint__sched_wakeup;

			break;

		case LOAD_MUTEX_RUN_SWITCH:
			s->prog_cnt = 4;

			s->progs[0].name = "mutex_lock_entry";
			s->progs[0].prog = &obj->progs.mutex_lock_entry;
			s->progs[0].link = &obj->links.mutex_lock_entry;

			s->progs[1].name = "mutex_lock_exit";
			s->progs[1].prog = &obj->progs.mutex_lock_exit;
			s->progs[1].link = &obj->links.mutex_lock_exit;

			s->progs[2].name = "mutex_unlock";
			s->progs[2].prog = &obj->progs.mutex_unlock;
			s->progs[2].link = &obj->links.mutex_unlock;

			s->progs[3].name = "raw_tracepoint__sched_wakeup";
			s->progs[3].prog = &obj->progs.raw_tracepoint__sched_switch;
			s->progs[3].link = &obj->links.raw_tracepoint__sched_switch;

			break;

		default:
			ret = 1;
			break;
	}

	return ret;
}

void perf_event_handler(int poll_fd, int map_fd, int array_fd, struct lockdetect_bpf *obj)
{
	int arg_key = 0, err = 0;
	struct arg_info arg_info = {};
	struct perf_buffer *pb = NULL;
	struct perf_buffer_opts pb_opts = {};

	pb_opts.sample_cb = lock_event;
	pb = perf_buffer__new(poll_fd, 64, &pb_opts);
	if (!pb) {
		err = -errno;
		fprintf(stderr, "failed to open perf buffer: %d\n", err);
		goto clean_syscall_slow;
	}

	arg_info.filter = env.filter;
	err = bpf_map_update_elem(map_fd, &arg_key, &arg_info, 0);
	if (err) {
		fprintf(stderr, "Failed to update arg_map\n");
		goto clean_syscall_slow;
	}

	err = lockdetect_bpf__attach(obj);
	if (err) {
		fprintf(stderr, "failed to attach BPF programs\n");
		goto clean_syscall_slow;
	}

	printf("start trace....\n");
	printf("时间  线程名;线程id;锁地址;等锁时间;持锁时间;调度延时;throttle延时;持锁上下文切换次数;线程所在的cgroup_id;  线程所在的cgroup路径名\n");


	while (!exiting) {
		err = perf_buffer__poll(pb, 100);
		if (err < 0 && err != -EINTR) {
			fprintf(stderr, "error polling perf buffer: %s\n", strerror(-err));
			goto clean_syscall_slow;
		}
		/* reset err to return 0 if exiting */
		err = 0;
	}

clean_syscall_slow:
	perf_buffer__free(pb);
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	if (level == LIBBPF_DEBUG && !debug_en)
		return 0;
	return vfprintf(stderr, format, args);
}

static void sig_exit(int signo)
{
	exiting = 1;
}

int main(int argc, char **argv)
{
	int err, ent_fd, arg_fd;
	struct lockdetect_bpf *obj;
	static const struct argp argp = {
		.options = opts,
		.parser = parse_arg,
		.doc = argp_program_doc,
	};

	debug_en = false;
	ksyms = NULL;

	if (access("/sys/kernel/btf/vmlinux", 0) != 0) {
		err = setenv("KAT_WORK_PATH", "/usr/local/kat/.kat_components", 1);
		if (err) {
			fprintf(stderr, "update KAT_WORK_PATH fail\n");
			return err;
		}
	}

    err = load_kallsyms(&ksyms);
    if (err) {
        fprintf(stderr, "Failed to load kallsyms\n");
		return err;
	}

	memset(&env.filter, 0, sizeof(struct filter));
	err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
	if (err) {
		fprintf(stderr, "argp_parse fail\n");
		return err;
	}
	env.filter.mode = load_mode;
	env.filter.cpuacct_hierarchy = cpuacct_hierarchy;

	if (env.filter.cpuacct_hierarchy)
		printf("Info: cpuacct_hierarchy %d \n", env.filter.cpuacct_hierarchy);

	if (load_mode == LOAD_RWSEM_RUN || load_mode == LOAD_MUTEX_RUN) {
		if (!env.filter.addr) {
			fprintf(stderr, "not specify lock addr\n");
			return 0;
		}
	}

	libbpf_set_print(libbpf_print_fn);

	bump_memlock_rlimit();

	obj = lockdetect_bpf__open();
	if (!obj) {
		fprintf(stderr, "failed to open  BPF object\n");
		err = 2;
		return err;
	}

	ebpf_dynamic_attach(obj, load_mode);
	bpf_object__load_skeleton(obj->skeleton);

	arg_fd = bpf_map__fd(obj->maps.arg_map);
	ent_fd = bpf_map__fd(obj->maps.events);
	stackmp_fd = bpf_map__fd(obj->maps.stackmap);
	fault_fd = 0;

	if (signal(SIGINT, sig_exit) == SIG_ERR ||
		signal(SIGALRM, sig_exit) == SIG_ERR) {
		fprintf(stderr, "can't set signal handler: %s\n", strerror(errno));
		err = 1;
		goto cleanup;
	}

	if (env.duration)
		alarm(env.duration);

	perf_event_handler(ent_fd, arg_fd, fault_fd, obj);

cleanup:
	lockdetect_bpf__destroy(obj);
	if (ksyms)
		free(ksyms);

	return err != 0;
}
