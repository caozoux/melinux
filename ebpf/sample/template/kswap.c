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
#include <sys/resource.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "kswap.h"
#include "./bpf/kswap.skel.h"

static int stackmp_fd;
static struct ksym *ksyms;
const char argp_program_doc[] =
"help \n";

struct filter {

};

struct env {
	time_t duration;
	bool verbose;
	struct filter filter;
} env;

volatile sig_atomic_t exiting = 0;

static const struct argp_option opts[] = {
	{ NULL, 'h', NULL, OPTION_HIDDEN, "Show the full help"},
	{},
};

static error_t parse_arg(int key, char *arg, struct argp_state *state)
{
	__u32 sysnr, pid;
	int i = 0, ret = errno;
	static int pos_args;
	char *tmp, *endptr;

	switch (key) {
	case 'h':
		argp_state_help(state, stderr, ARGP_HELP_STD_HELP);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static void sig_exit(int signo)
{
	exiting = 1;
}

int main(int argc, char *argv[])
{
	int err, ent_fd, arg_fd;
	int arg_key;
	struct kswap_bpf *obj;
	struct arg_info arg_info = {};
	struct perf_buffer *pb = NULL;
	struct perf_buffer_opts pb_opts = {};
	static const struct argp argp = {
		.options = opts,
		.parser = parse_arg,
		.doc = argp_program_doc,
	};

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

	obj = kswap_bpf__open_and_load();
	if (!obj) {
		fprintf(stderr, "failed to open  BPF object\n");
		err = 2;
		return err;
	}

	arg_fd = bpf_map__fd(obj->maps.arg_map);
	ent_fd = bpf_map__fd(obj->maps.events);
	stackmp_fd = bpf_map__fd(obj->maps.stackmap);

	err = bpf_map_update_elem(arg_fd, &arg_key, &arg_info, 0);
	if (err) {
		fprintf(stderr, "Failed to update arg_map\n");
		goto clean_syscall_slow;
	}

	if (signal(SIGINT, sig_exit) == SIG_ERR ||
		signal(SIGALRM, sig_exit) == SIG_ERR) {
		fprintf(stderr, "can't set signal handler: %s\n", strerror(errno));
		err = 1;
		goto cleanup;
	}

	err = kswap_bpf__attach(obj);
	if (err) {
		fprintf(stderr, "failed to attach BPF programs\n");
		goto clean_syscall_slow;
	}

clean_syscall_slow:
	perf_buffer__free(pb);
	
cleanup:
	kswap_bpf__destroy(obj);
	if (ksyms)
		free(ksyms);

	return 0;
}
