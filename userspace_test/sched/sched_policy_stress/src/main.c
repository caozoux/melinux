#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include<unistd.h>
#include<pthread.h>
#include <fcntl.h>
#include<unistd.h>
#include<sys/time.h>
#include<semaphore.h>
#include<linux/sched.h>

#define MAX_TRHEAD  (128)

static unsigned long run_val = 1UL<<20;
unsigned long args_thread_cnt = 0;
int arg_cpu_util;

struct thread_data {
	unsigned long id;
	unsigned long run_val_ms;
};
enum test_case {
	CASE_NULL,
	CASE_BALANCE,
};

pthread_t thread_array[MAX_TRHEAD];
struct thread_data thread_data_array[MAX_TRHEAD];
static unsigned long cpu_10_ms_count;
static int args_sched_policy = 0;
static int args_sched_priority = -1;
static enum test_case args_test_case = CASE_NULL;

static void cpu_10_ms_time(void)
{
	unsigned long cnt = cpu_10_ms_count;
	while(cnt--);
}

void change_sched_policy(void)
{
	struct sched_param my_params;
	int policy = SCHED_NORMAL;
	int rc;

	if (args_sched_policy) {
		rc = sched_setscheduler(0, args_sched_policy, &my_params);
		printf("set sched policy:%d priority:%d\n", args_sched_policy, my_params.sched_priority);
		if(rc<0)
		{
			//perror("sched_setscheduler to %d error", args_sched_policy);
			perror("sched_setscheduler error");
			exit(0);
		}
	}
}

void *threadFunc(void *param)
{
	struct thread_data *data = (struct thread_data *)param;
	int i;
	int sleep_ms = 1000 - arg_cpu_util * 10;

	while(1) {
		for (i = 0; i < arg_cpu_util; ++i) {
			cpu_10_ms_time();
		}
		usleep(sleep_ms*1000);
	}
}

void test_case_balance()
{
	int i, cnt = 5;
	arg_cpu_util = 100;

	for (i = 0; i < cnt; ++i) {
		thread_data_array[i].id = i;
		thread_data_array[i].run_val_ms = 0;
		pthread_create(&thread_array[i], NULL, threadFunc, &thread_data_array[i]);
		sleep(5);
	}

	for (i = 0; i < cnt; ++i) {
		pthread_join(thread_array[i], NULL);
	}
}

void run_test_cases(enum test_case case_item)
{
	switch (case_item) {
		case CASE_BALANCE:
			test_case_balance();
			break;
		default:
			break;
	}
}

void help(void)
{
	printf("-t/--threads    run thread numbers\n");
	printf("-u/--cpu_util   per thread cpu util(1-100)\n");
	printf("-p/--sched_policy fifo/deadline/cpuidle/qos\n");
	printf("-c/--case         balance\n");
}

static void caculte_cpu_10_counts(void)
{
	struct timeval tvafter,tvpre;
	unsigned long ts;
	unsigned long cnt = 1UL<<24;

    gettimeofday (&tvpre , NULL);

	while(cnt--);

    gettimeofday (&tvafter, NULL);
	ts = (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000;
	cpu_10_ms_count = ((1UL<<24)/ts)*10;
}

int main(int argc, char *argv[])
{
	int choice;
	int i;

	args_thread_cnt = 0;

	caculte_cpu_10_counts();
	if (argc < 2) {
		help();
		return 0;
	}

	while (1)
	{
		static struct option long_options[] =
		{
			/* Use flags like so:
			{"verbose",	no_argument,	&verbose_flag, 'V'}*/
			/* Argument styles: no_argument, required_argument, optional_argument */
			{"version", no_argument,	0,	'v'},
			{"help",	no_argument,	0,	'h'},
			{"threads",	required_argument,	0,	't'},
			{"cpu_util",required_argument,	0,	'u'},
			{"sched_policy", required_argument,	0,	'p'},
			{"cases", required_argument,	0,	'c'},
			{0,0,0,0}
		};
	
		int option_index = 0;
		choice = getopt_long( argc, argv, "vht:u:p:c:", long_options, &option_index);
	
		if (choice == -1)
			break;
	
		switch( choice )
		{
			case 'v':
				break;
	
			case 'h':
				help();
				return 0;
	
			case 't':
				args_thread_cnt = atoi(optarg);
				break;

			case 'p':
				if (strstr("fifo", optarg))
					args_sched_policy = SCHED_FIFO;
				else if (strstr("deadline", optarg))
					args_sched_priority = SCHED_DEADLINE;
				else if (strstr("cpuidle", optarg))
					//sched_priority = SCHED_IDLE;
					args_sched_policy = SCHED_IDLE;
				else if (strstr("rr", optarg))
   					args_sched_policy = SCHED_RR;
				else if (strstr("qos", optarg))
					args_sched_policy = 7;
				else if (strstr("case", optarg))
					args_sched_policy = 7;
				else {
					printf("sched policy not support\n");
					exit(1);
				}
				/* getopt_long will have already printed an error */
				break;
	
			case 'u':
				arg_cpu_util = atoi(optarg);
				if (arg_cpu_util > 100) {
					printf(stderr, " cpu_util must be litter 100\n");
					return -1;
				}
				break;

			case 'c':
				if (strstr("balance", optarg))
					args_test_case = CASE_BALANCE;
				else {
					printf("test case not support\n");
					exit(1);
				}
	
			case '?':
				/* getopt_long will have already printed an error */
				break;
	
			default:
				/* Not sure how to get here... */
				return -1;
		}
	}
	
	if (args_sched_policy)
		change_sched_policy();

	if (args_test_case) {
		run_test_cases(args_test_case);
		return 0;
	}

	if (args_thread_cnt) {
		for (i = 0; i < args_thread_cnt; ++i) {
			thread_data_array[i].id = i;
			thread_data_array[i].run_val_ms = 0;
			pthread_create(&thread_array[i], NULL, threadFunc, &thread_data_array[i]);
		}

		for (i = 0; i < args_thread_cnt; ++i) {
			pthread_join(thread_array[i], NULL);
		}
	}

	return 0;
}
