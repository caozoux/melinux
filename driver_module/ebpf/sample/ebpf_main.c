#include "bpf_load.h"
#include <stdio.h>
#include <unistd.h>

struct task_write {
	char name[256];
	unsigned long write_size;
};

int main(int argc, char **argv)
{
	long key, next_key;
	struct task_write value;
	if (load_bpf_file("ebpf_module_kprobe.o") != 0) {
		printf("The kernel didn't load the BPF program\n");
    	return -1;
	}
	sleep(5);
	while (bpf_map_get_next_key(map_fd[0], &key, &next_key) == 0) {
			bpf_map_lookup_elem(map_fd[0], &next_key, &value);
			printf("zz %s:%d \n", value.name, value.write_size);
			key=next_key;
	}

    //read_trace_pipe();

  return 0;
}
