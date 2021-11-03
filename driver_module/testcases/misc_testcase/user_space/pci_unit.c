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
	printf("pci --enum   scan all pci device\n");
	printf("pci --dumpresource  show all resource\n");
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"enum",     no_argument, 0,  0 },
	{"dumpresource", required_argument, 0,  0 },
	{"writeresource", required_argument, 0,  0 },
	{"offset", required_argument, 0,  0 },
	{"valule", required_argument, 0,  0 },
	//{"enum",     required_argument, 0,  0 },
	{0,0,0,0}
};

int pci_usage(int argc, char **argv)
{
	struct ioctl_data data;
	struct pci_data *p_data;
	int ret;
	int c;

	if (argc <= 1) { 
		help();
		return 0;
	}

	p_data = &data.pci_data;
	data.type = IOCTL_PCI;
	while (1) {
		int option_index = -1;

		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (option_index) {
			case 0:
				help();
				return 0;
			case 1:
				data.cmdcode = IOCTL_PCI_ENUM;
				ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				return ret;

			case 2:
				data.cmdcode = IOCTL_PCI_ENUM;
				ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				return ret;

			default:
				break;
		}
	}

	return 0;
}
