#include <iostream>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ksioctl/kinject_ioctl.h>

#include "kapp_unit.h"

int hrtimer_args_handle(int argc, char **argv);
int statickey_args_handle(int argc, char **argv);
int slub_args_handle(int argc, char **argv);
int rwsem_args_handle(int argc, char **argv);
int stack_args_handle(int argc, char **argv);
int lock_args_handle(int argc, char **argv);
int mutex_args_handle(int argc, char **argv);
int fault_args_handle(int argc, char **argv);

struct cmdargs kinject_args[] = {
	{"--statickey",statickey_args_handle, 
		"\n"
		"     -e enable \n"
		"     -d disable \n"
	},
	{"--hrtimer", hrtimer_args_handle,
		"\n"
		"     --en enable \n"
		"     --dis disable\n"
		"     -t timer mis vaul\n"
	},
	{"--slub", slub_args_handle,
		"\n"
		"     --en enable \n"
		"     --dis disable\n"
		"     --l_overwrite inject slub overwrite\n"
		"     --r_overwrite inject slub overwrite\n"
		"     --double_free inject slub overwrite\n"
	},
	{"--rwsem", rwsem_args_handle,
		"\n"
		"     --wrdown enable \n"
		"     --wrup enable \n"
		"     --rddown enable \n"
		"     --rdup enable \n"
		"     --mmap_sem_downwrite \n"
		"     --mmap_sem_upwrite \n"
		"     --msg  print out msg\n"
	},
	{"--stack", stack_args_handle,
		"\n"
		"     --overwrite inject stack overwrite\n"
	},
	{"--lock", lock_args_handle,
		"\n"
		"     --lock  spinlock\n"
		"     --ilock spinlock with irq\n"
		"     --time  mesecond\n"
	},
	{"--mutex", mutex_args_handle,
		"\n"
		"     --mutex_lock    mutex lock\n"
		"     --mutex_unlock  mutex unlock\n"
		"     --mutex_dlock mesecond  mutex lock and unlock with mesecond\n"
	},
	{"--fault", fault_args_handle,
		"\n"
		"     --softlockup \n"
		"     --rcu_hungmutex \n"
		"     --task_hung \n"
		"     --warning \n"
		"     --list_corrupt \n"
	},
};

int fault_args_handle(int argc, char **argv)
{
	int c;
	int ret;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;
	static int fault_softlockup, fault_rcu_hung, fault_task_hung;
	static int fault_waring, fault_list_corrupt;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	static struct option lock_opts[] = {
		{ "softlockup",no_argument, &fault_softlockup,1},
		{ "rcu_hung",no_argument, &fault_rcu_hung,1},
		{ "task_hung",no_argument, &fault_task_hung,1},
		{ "warning",no_argument, &fault_waring,1},
		{ "list_corrupt",no_argument, &fault_list_corrupt,1},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", lock_opts, NULL)) != -1)
	{
		switch(c) {
			default:
				break;
		}
	}

	if (fault_softlockup)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SOFTWATCHDOG_TIMEOUT,
				&data, sizeof(struct ioctl_ksdata));
	if (fault_rcu_hung)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_RUC_HUNG,
				&data, sizeof(struct ioctl_ksdata));
	if (fault_task_hung)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_TASK_HUNG,
				&data, sizeof(struct ioctl_ksdata));
	if (fault_waring)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_WARN,
				&data, sizeof(struct ioctl_ksdata));
	if (fault_list_corrupt)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_LIST_CORRUPT,
				&data, sizeof(struct ioctl_ksdata));

	return ret;
}

int mutex_args_handle(int argc, char **argv)
{
	int c;
	int ret;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;
	static int mutex_lock_en, mutex_unlock_en, mutex_dlock_en;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);
	kinject_data.lock.lock_ms = 1000;

	static struct option lock_opts[] = {
		{ "mutex_lock",no_argument, &mutex_lock_en,1},
		{ "mutex_unlock",no_argument, &mutex_unlock_en,1},
		{ "mutex_dlock",required_argument, NULL,'t'},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", lock_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				mutex_dlock_en = 1;
				kinject_data.lock.lock_ms = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (mutex_lock_en)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_MUTEXT_LOCK,
				&data, sizeof(struct ioctl_ksdata));
	if (mutex_unlock_en)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_MUTEXT_UNLOCK,
				&data, sizeof(struct ioctl_ksdata));
	if (mutex_dlock_en)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_MUTEXT_DEALY,
				&data, sizeof(struct ioctl_ksdata));

	return ret;
}

int rwsem_args_handle(int argc, char **argv)
{
	int c;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;

	static int rwsem_wr_down,rwsem_wr_up;
	static int rwsem_rd_down,rwsem_rd_up;
	static int rwsem_mmapsem_downwrite, rwsem_mmapsem_upwrite;
	int time=0;
	int ret;
	char *msg = NULL;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	static struct option slub_opts[] = {
		{ "wrdown",no_argument,&rwsem_wr_down,1},
		{ "wrup",no_argument,&rwsem_wr_up,1},
		{ "rddown",no_argument,&rwsem_rd_down,1},
		{ "rdup",no_argument,&rwsem_rd_up,1},
		{ "mmap_sem_downwrite",no_argument,&rwsem_mmapsem_downwrite,1},
		{ "mmap_sem_upwrite",no_argument,&rwsem_mmapsem_upwrite,1},
		{ "msg",required_argument,NULL,'m'},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", slub_opts, NULL)) != -1)
	{
		switch(c) {
			case 'm':
				msg = optarg;
				break;
			default:
				break;
		}
	}

	if (rwsem_wr_down)
		ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_RWSEM_WRITEDOWN,
				&data, sizeof(struct ioctl_ksdata));

	if (rwsem_wr_up)
		ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_RWSEM_WRITEUP,
				&data, sizeof(struct ioctl_ksdata));

	if (rwsem_rd_up)
		ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_RWSEM_READUP,
				&data, sizeof(struct ioctl_ksdata));

	if (rwsem_rd_down)
		ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_RWSEM_READDOWN,
				&data, sizeof(struct ioctl_ksdata));

	//if (rwsem_mmapsem_downwrite)
    //		ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_MMAP_SEM_WRITEDWON,
    //				&data, sizeof(struct ioctl_ksdata));

	//if (rwsem_mmapsem_upwrite)
    //		ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_MMAP_SEM_WRITEUP,
	//			&data, sizeof(struct ioctl_ksdata));

    if (msg)
		printf("%s\n", msg);
	while(1) {
		sleep(1);
	}
	return 0;
}

int lock_args_handle(int argc, char **argv)
{
	int c;
	int ret;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;
	static int lock_enable, ilock_enable;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);
	kinject_data.lock.lock_ms = 1000;

	static struct option lock_opts[] = {
		{ "lock",no_argument, &lock_enable,1},
		{ "ilock",no_argument, &ilock_enable,1},
		{ "time",required_argument, NULL,'t'},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", lock_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				kinject_data.lock.lock_ms = atoi(optarg);
				break;
			default:
				break;
		}
	}

	printf("zz %s lock_ms:%ld \n",__func__, (unsigned long)kinject_data.lock.lock_ms);
	if (lock_enable)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SPINLOCK_DEPLOCK,
				&data, sizeof(struct ioctl_ksdata));
	if (ilock_enable)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_IRQSPINLOCK_DEPLOCK,
				&data, sizeof(struct ioctl_ksdata));

	return ret;
}

int stack_args_handle(int argc, char **argv)
{
	int c;
	int ret;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;
	static int stack_overwrite;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	static struct option slub_opts[] = {
		{ "overwrite",no_argument,&stack_overwrite,1},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", slub_opts, NULL)) != -1)
	{
		switch(c) {
			default:
				break;
		}
	}

	if (stack_overwrite)
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_STACK_OVERWRITE,
				&data, sizeof(struct ioctl_ksdata));

	return ret;
}

int slub_args_handle(int argc, char **argv)
{
	int c;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;

	static int slub_enable, slub_disable;
	static int slub_l_overwrite, slub_r_overwrite, slub_dfree;
	int time=0;
	int ret;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	static struct option slub_opts[] = {
		{ "en",no_argument,&slub_enable,1},
		{ "dis",no_argument,&slub_disable,1},
		{ "l_overwrite",no_argument,&slub_l_overwrite,1},
		{ "r_overwrite",no_argument,&slub_r_overwrite,1},
		{ "double_free",no_argument,&slub_dfree,1},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", slub_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				time = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (slub_enable) {
		kinject_data.enable = 1;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_CTRL,
				&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject start slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (slub_disable) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_CTRL,
				&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (slub_l_overwrite) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_L_OVERWRITE,
				&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (slub_r_overwrite) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_L_OVERWRITE,
				&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (slub_dfree) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_DOUBLE_FREE,
				&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	return 0;
}
int statickey_args_handle(int argc, char **argv)
{
	static int key_enable, key_disable;
	static struct option key_opts[] = {
		{ "enable",no_argument,&key_enable,'e'},
		{ "disable",no_argument,&key_disable,'d'},
		{     0,    0,    0,    0},
	};
	int c;

	while((c = getopt_long(argc, argv, ":l:", key_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				break;
			default:
				break;
		}
	}
	return 0;
}

int hrtimer_args_handle(int argc, char **argv)
{
	int c;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;

	static int hrtimer_enable, hrtimer_disable;
	int time=0;
	int ret;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	static struct option hrtimer_opts[] = {
		{ "en",no_argument,&hrtimer_enable,1},
		{ "dis",no_argument,&hrtimer_disable,1},
		{ "time",required_argument,NULL,'t'},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", hrtimer_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				time = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (hrtimer_enable) {
		kinject_data.enable = 1;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER,
				&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject start hrtimer failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (hrtimer_disable) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER,
				&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop hrtimer failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (time)
		printf("zz %s %d %d\n", __func__, __LINE__, time);

	return 0;
}

int unit_kinject(int argc, char** argv)
{
#if 0
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	if (FLAGS_kinjectunit == "statickey") {
		kinject_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_TEST , &data, sizeof(struct ioctl_ksdata));
	}

	if (FLAGS_kinjectunit == "htitmer_s") {
		kinject_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER, &data, sizeof(struct ioctl_ksdata));
	}

	if (FLAGS_kinjectunit== "htitmer_d") {
		kinject_data.enable = 0;
		return ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER, &data, sizeof(struct ioctl_ksdata));
	}
#else
#endif
	int i;
	int is_help = 0;
	char *command;

	if (argc < 2) {
		return 0;
	}

	if (strstr(argv[1], "help"))  {
		is_help=1;
	}

	command = argv[1];
	for (i = 0; i < sizeof(kinject_args) / sizeof(struct cmdargs); i++) {
		if (is_help) {
			printf("    %s: %s\n", kinject_args[i].name, kinject_args[i].help);
		} else {
			if(!strcmp(command, kinject_args[i].name)) {
				argc--;
				argv++;
				return kinject_args[i].func(argc, argv);
			}
		}
	}

	return 0;
}
