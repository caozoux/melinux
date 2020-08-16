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


#define DEV_NAME "/dev/misc_template"

typedef int (*misc_fp)(int argc, char **argv);

struct memisc_func {
    const char* name;
    misc_fp func;
};

struct ping_data {
	short unsigned int  s_send;
	short unsigned int  s_recv;
};

int misc_fd;

static int usage_help(int argc, char **argv);
void usage_limit_help()
{
	printf("help:  -t [optcode:softlock/hwlock/mem/rcu] -o optcode\n");
	printf("       -s softlock: test softlock \n");
	printf("       -w hwlock:   hw lock \n");
	printf("          1:   hw lock \n");
	printf("          2:   hw unlock \n");
	printf("          3:   hw trylock \n");
	printf("          4:   hw irqlock \n");
	printf("          5:   hw irqunlock \n");
	printf("          6:   hw irqtrylock \n");
	printf("       -r rcu:      hw lock \n");
	printf("       -m mem:      test mem\n");
	printf("       -e tree      tree test\n");
	printf("          radix add index  raidixtree add inode\n");
	printf("          radix del index  raidixtree del inode\n");
	printf("          radix get index  raidixtree get inode\n");
	printf("          radix dump index raidixtree dump inode\n");
	printf("       -q workqueue:      test workqueue\n");
	printf("          1 timemisc:   run workqueue\n");
	printf("          2 timemisc:   run workqueue spinlock\n");
	printf("          3 timemisc:   test workqueue spinlockirq\n");
	printf("          4 timemisc:   test workqueue  percpu spinlockirq\n");
	printf("          5 timemisc:   caculte the workqueue time from queue to run\n");
	printf("       -k funcname  dumpstack of kernel function \n");
	printf("       -a atomic    atomic performance\n");
	printf("       mem:         -o  dump    show the page pgd pud pmd pte info\n");
	printf("       mem:         -o  vmmax   test the max vmlloc support \n");
}

static int raidix_test(int fd ,struct ioctl_data *data, char *ch1, char *ch2, char *ch3)
{
		int index;

		data->type = IOCTL_USERAIDIXTREE;

		if (ch1 == NULL || ch2 == NULL
				||ch3 == NULL)
			return 1;

		index = atoi(ch3);
		data->raidix_data.index = index;

		if (!strcmp(ch1,"radix")) {
			if (!strcmp(ch2,"add")) {
				data->cmdcode = IOCTL_USERAIDIXTREE_ADD;
			} else if (!strcmp(ch1,"del")) {
				data->cmdcode = IOCTL_USERAIDIXTREE_DEL;
			} else if (!strcmp(ch1,"get")) {
				data->cmdcode = IOCTL_USERAIDIXTREE_GET;
			} else if (!strcmp(ch1,"dump")) {
				data->cmdcode = IOCTL_USERAIDIXTREE_DUMP;
			} else {
				return 1;
			}
		} else if (!strcmp(ch1,"rbtree")) {
		} else {
			return 1;
		}
		return ioctl(fd, sizeof(struct ioctl_data), data);
}

static int workqueue_test(int fd ,struct ioctl_data *data, char *ch1, char *ch2)
{
		char ch = *ch1;
		unsigned long workqueue_wakeup_time;
		int ret;
		int runtime = atoi(ch2);
		data->type = IOCTL_USEWORKQUEUE;
		data->wq_data.runtime = runtime;
		data->wq_data.workqueue_performance = &workqueue_wakeup_time;
		*data->wq_data.workqueue_performance = 0;
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
			case '4':
				data->cmdcode =  IOCTL_USEWORKQUEUE_PERCPU_SPINLOCKIRQ_RACE;
				break;
			case '5':
				data->cmdcode =  IOCTL_USEWORKQUEUE_PEFORMANCE_DELAY;
				break;
			default:
				return 1;
		}

		ret = ioctl(fd, sizeof(struct ioctl_data), data);
		if (data->cmdcode ==  IOCTL_USEWORKQUEUE_PEFORMANCE_DELAY)
			printf("the workqueue wakeup time:%ld \n", *(data->wq_data.workqueue_performance));
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

static int atomic_test(int fd ,struct ioctl_data *data)
{
	return ioctl(fd, sizeof(struct ioctl_data), data);
}


static int normal_usage(int argc, char **argv)
{
	struct ioctl_data data;
	char ch;
	int ret;
	char name[128];
    while((ch=getopt(argc,argv,"hsw:rm:k:q:a"))!=-1)
  	{
		switch (ch) {
			case 'h':
				usage_limit_help();
				break;

			case 'a':
				data.type = IOCTL_USEATOMIC ;
				data.cmdcode = IOCTL_USEATOMIC_PERFORMANCE;
				atomic_test(misc_fd, &data);
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
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);

			case 'e':
				ruc_test(misc_fd);
				goto out;

			case 'r':
				ruc_test(misc_fd);
				goto out;

			case 'm':
				data.type = IOCTL_MEM;
				data.cmdcode = IOCTL_TYPE_VMALLOC_MAX;
				break;

			case 'q':
				ret = workqueue_test(misc_fd, &data, argv[optind-1], argv[optind]);
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
				ret = kprobe_test(misc_fd, &data);
				if (ret) {
					printf("kprobe failed\n");
					return 0;
				}

				printf("%s\n", data.kp_data.dump_buf);

				break;
			default:
				usage_limit_help();
				return 0;
		}
	}

out:
	return -1;
}

static struct memisc_func all_funcs[] = {
	{"help", usage_help},
	{"normal", normal_usage},
	{"kmem", kmem_usage},
	{"block", block_usage},
};

static int usage_help(int argc, char **argv)
{
#if 0
	static const struct option long_options[] = {
		{"input",     required_argument, 0,  0 },
		{"output",     required_argument, 0,  0 },
		{"output",     required_argument, 0,  0 },
		{0,0,0,0}};
	int c;
	int __attribute__ ((unused)) ret;

	while (1) {
		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if ( c == -1 )
			break;
		if (option_index == 0) {
			usage_help();
		} else if (option_index == 1) {
			
		}

	}
#else
	usage_limit_help();
	printf("kmem --help\n");
	printf("block --help\n");
#endif
}

int main(int argc, char *argv[])
{
	int i;
	if (argc < 1) {
		usage_help(argc, argv);
		return 0;
	}

	misc_fd = open(DEV_NAME, O_RDWR);
	if (misc_fd <= 0) {
		printf("%s open failed", DEV_NAME);
		return 0;
	}

	for (i = 0; i < sizeof(all_funcs) / sizeof(struct memisc_func); i++) {
		if (strcmp(argv[1], all_funcs[i].name) == 0) {
			all_funcs[i].func(argc - 1, argv + 1);
			break;
		}
	}


out:
	close(misc_fd);
	return 0;
}
