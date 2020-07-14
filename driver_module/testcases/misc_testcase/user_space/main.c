#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "template_iocmd.h"
#include "common_head.h"


#define DEV_NAME "/dev/misc_template"

struct ping_data {
	short unsigned int  s_send;
	short unsigned int  s_recv;
};

void usage_help()
{
	printf("help:  -t [optcode:softlock/hwlock/mem/rcu] -o optcode\n");
	printf("       -s softlock: test softlock \n");
	printf("       -w hwlock:   hw lock \n");
	printf("       -w 1:   hw lock \n");
	printf("       -w 2:   hw unlock \n");
	printf("       -w 3:   hw trylock \n");
	printf("       -w 4:   hw irqlock \n");
	printf("       -w 5:   hw irqunlock \n");
	printf("       -w 6:   hw irqtrylock \n");
	printf("       -r rcu:      hw lock \n");
	printf("       -m mem:      test mem\n");
	printf("       -q workqueue:      test workqueue\n");
	printf("       -q 1 timemisc:   run workqueue\n");
	printf("       -q 2 timemisc:   run workqueue spinlock\n");
	printf("       -q 3 timemisc:   test workqueue spinlockirq\n");
	printf("       -k funcname  dumpstack of kernel function \n");
	printf("       mem:         -o  dump    show the page pgd pud pmd pte info\n");
	printf("       mem:         -o  vmmax   test the max vmlloc support \n");
}

static int workqueue_test(int fd ,struct ioctl_data *data, char *ch1, char *ch2)
{
		char ch = *ch1;
		int runtime = atoi(ch2);
		data->type = IOCTL_USEWORKQUEUE;
		data->wq_data.runtime = runtime;
		switch (ch) {
			case '1':
				data->cmdcode = IOCTL_USEWORKQUEUE_SIG;
				break;
			case '2':
				data->cmdcode = IOCTL_USEWORKQUEUE_SIG_SPINLOCK;
				break;
			case '3':
				data->cmdcode = IOCTL_USEWORKQUEUE_SIG_SPINLOCKIRQ;
				break;
			default:
				return 1;
		}
		printf("%s\n", optarg);
		return ioctl(fd, sizeof(struct ioctl_data), data);
}

static int hardlock_test(int fd, struct ioctl_data *data)
{
		data->type = IOCTL_HARDLOCK;
		data->cmdcode = IOCTL_HARDLOCK;
		printf("%s\n", optarg);
		return ioctl(fd, sizeof(struct ioctl_data), data);
}

static int kprobe_test(int fd ,struct ioctl_data *data)
{
	return ioctl(fd, sizeof(struct ioctl_data), data);
}

int main(int argc, char *argv[])
{
	struct ioctl_data data;
	char ch;
	int fd;	
	int ret;
	char name[128];

	if (argc == 1) {
		usage_help();
		return 0;
	}

	fd = open(DEV_NAME, O_RDWR);
	if (fd <= 0) {
		printf("%s open failed", DEV_NAME);
		return 0;
	}

    while((ch=getopt(argc,argv,"hsw:rm:k:q:"))!=-1)
  	{
		switch (ch) {
			case 'h':
				usage_help();
				break;
			case 's':
				data.type = IOCTL_SOFTLOCK;
				break;

			case 'w':
				ch = optarg[0];
				data.type = IOCTL_HARDLOCK;
				switch (ch) {
					case '1':
						data.cmdcode = IOCTL_HARDLOCK_LOCK;
						break;
					case '2':
						data.cmdcode = IOCTL_HARDLOCK_UNLOCK;
						break;
					case '3':
						data.cmdcode = IOCTL_HARDLOCK_TRYLOCK;
						break;
					case '4':
						data.cmdcode = IOCTL_HARDLOCK_IRQLOCK;
						break;
					case '5':
						data.cmdcode = IOCTL_HARDLOCK_IRQUNLOCK;
						break;
					case '6':
						data.cmdcode = IOCTL_HARDLOCK_IRQTRYLOCK;
						break;
					default:
						printf("hardlock operation not support\n");
						return -1;
				}
				return ioctl(fd, sizeof(struct ioctl_data), &data);

			case 'r':
				ruc_test(fd);
				goto out;

			case 'm':
				data.type = IOCTL_MEM;
				data.cmdcode = IOCTL_TYPE_VMALLOC_MAX;
				break;

			case 'q':
				ret = workqueue_test(fd, &data, argv[optind-1], argv[optind]);
				break;

			case 'k':
				data.type = IOCTL_USEKPROBE;
				data.cmdcode = IOCTL_USEKRPOBE_FUNC_DUMP;
				printf("%s\n", optarg);
				strcpy(name, optarg);
				data.kp_data.name = name;
				data.kp_data.len = strlen(name);
				data.kp_data.dump_buf= malloc(4096);
				data.kp_data.dump_len= 4096;
				ret = kprobe_test(fd, &data);
				if (ret) {
					printf("kprobe failed\n");
					return 0;
				}

				printf("%s\n", data.kp_data.dump_buf);

				break;
			default:
				usage_help();
				return 0;
		}
	}


out:
	close(fd);
	return 0;
}
