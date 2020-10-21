#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include "template_iocmd.h"
#include "common_head.h"

extern int misc_fd;

static void help(void)
{
	printf("kmem --dump  get the vm_state of zone/none/numa\n");
	printf("kmem --vma_scan --pid scan the vma of current task\n");
}

int kmem_usage(int argc, char **argv)
{
	static const struct option long_options[] = {
		{"help",     no_argument, 0,  0 },
		{"dump",     required_argument, 0,  0 },
		{"output",     required_argument, 0,  0 },
		{"vma_scan",   no_argument, 0,  0 },
		{"pid",   required_argument, 0,  0 },
		{0,0,0,0}};
	int c;
	struct ioctl_data data;
	int  __attribute__ ((unused)) ret = 0;
	char pidstr[128];
	int  pid = -1; 
	unsigned long zone_vm_stat[64];
	unsigned long numa_vm_stat[64];
	unsigned long node_vm_stat[64];

	ioctl_data_init(&data);

	data.type = IOCTL_USEKMEM;
	data.kmem_data.zone_vm_state = zone_vm_stat;
	data.kmem_data.zone_len = 4*64;
	data.kmem_data.node_vm_state = node_vm_stat;
	data.kmem_data.node_len = 4*64;
	data.kmem_data.numa_vm_state = numa_vm_stat;
	data.kmem_data.numa_len = 4*64;

	while (1) {
		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1) {
			help();
			break;
		}

		switch (option_index) {
			case 0:
				help();
				break;
			case 1:
				data.cmdcode = IOCTL_USEKMEM_GET;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;
			case 2:
				data.cmdcode = IOCTL_USEKMEM_GET;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;
			case 3:
				data.cmdcode = IOCTL_USEKMEM_VMA_SCAN;
				//return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;
			case 4:
				strncpy(pidstr, optarg, 128);
				pid = atoi(pidstr);
				break;
			default:
				break;
		}
	}

	if (data.cmdcode == IOCTL_USEKMEM_VMA_SCAN) {
		data.pid = pid;
		ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
		printf("zz %s data.log_buf:%s \n",__func__, data.log_buf);
	}
	
	return ret;
}
