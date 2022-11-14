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

static int usage_help(int argc, char **argv);
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
char *glog_buf;

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

static unsigned long get_kallsyms_lookup_address(void)
{
#define BUF_SIZE (0x1000000)
    char *buf;
    int fd;
    unsigned long ret = 0;
    int i, len;
    char kallsyms_str[17]={0};
    char *str;
    buf= malloc(BUF_SIZE);
    if (!buf)
        return ret;

    fd=open("/proc/kallsyms", O_RDONLY);
    if (fd<0) {
        free(malloc);
        return ret;
    }

    len = 0;
    str = buf;
    do {
        len = read(fd, str, BUF_SIZE);
        str += len;
    } while(len > 0);
    //str=strstr(buf, " T kallsyms_lookup_address\n");
    str=strstr(buf, " T kallsyms_lookup_name");
    if (!str)
        goto out;

    str -= 16;
    strncpy(kallsyms_str, str, 16);
	kallsyms_str[16] = 0;

    ret = strtoull(kallsyms_str, NULL, 16);

out:
    free(buf);
    close(fd);
    return ret;
}

int env_init(void)
{
	struct ioctl_data data;
	unsigned long address;

	ioctl_data_init(&data);
	data.type = IOCTL_INIT;
	address = get_kallsyms_lookup_address();
	if (!address)
		return 0;
	
	data.init_data.kallsyms_func = address;
	return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
}

static struct memisc_func all_funcs[] = {
	{"help", usage_help},
	{"kmem", kmem_usage},
	{"ktime", ktime_usage},
	{"block", block_usage},
	{"lock", lock_usage},
	{"workqueue", workqueue_usage},
	{"rcu", rcu_usage},
	{"kprobe", kprobe_usage},
	{"radixtree", radixtree_usage},
	{"sched", ksched_usage},
	{"pci", pci_usage},
	{"panic", panic_usage},
	{"inject", inject_usage},
	{"diagnose", kdiagnose_usage},
};

static int usage_help(int argc, char **argv)
{
	int i;
	char *help[2]  = { "help", NULL};
	printf("init  initailziation module\n");
	for (i = 0; i < sizeof(all_funcs) / sizeof(struct memisc_func); i++) {
			if (strcmp("help", all_funcs[i].name) != 0) {
				all_funcs[i].func(argc - 1, help);
			}
	}
}

void ioctl_data_init(struct ioctl_data *data)
{
	data->log_buf = glog_buf;
	memset(glog_buf, 0, 8192);
}

int main(int argc, char *argv[])
{
	int i;
	int ret = 0;

	glog_buf = malloc(8192);
	memset(glog_buf, 0, 8192);

	if (argc < 2) {
		usage_help(argc, argv);
		goto out1;
	}

	misc_fd = open(DEV_NAME, O_RDWR);
	if (misc_fd <= 0) {
		printf("%s open failed", DEV_NAME);
		return 0;
	}

	for (i = 0; i < sizeof(all_funcs) / sizeof(struct memisc_func); i++) {
		if (strcmp(argv[1], "init") == 0) {
			printf("run init\n");
			env_init();
			break;
		} else if (strcmp(argv[1], all_funcs[i].name) == 0) {
			all_funcs[i].func(argc - 1, argv + 1);
			break;
		}
	}



out:
	close(misc_fd);
out1:
	free(glog_buf);
	return ret;
}
