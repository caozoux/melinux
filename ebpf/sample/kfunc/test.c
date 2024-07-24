#include <bpf/bpf.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <linux/icmp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "example.skel.h"

int main (int argc, char *argv[])
{
	int ret;
	struct example_bpf *prog = example_bpf__open_and_load();
	if (!prog) {
		printf("[error]: failed to open and load program.\n");
		return -1;
	}

	ret = example_bpf__attach(prog);
	if (ret) {
		perror("bpf attach failed");
		goto out;
	}
	sleep(30);
	printf("[success] pass non-ICMP packets\n");

	example_bpf__detach(prog);
out:
	example_bpf__destroy(prog);
	return 0;
}
