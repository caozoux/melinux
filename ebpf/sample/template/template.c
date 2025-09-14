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

const char argp_program_doc[] =
"help \n";


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

int main(int argc, char *argv[])
{
	
	return 0;
}
