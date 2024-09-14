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

#define MAX_TRHEAD  (128)

static unsigned long run_val = 1UL<<20;
unsigned long thread_cnt;
int arg_cpu_util;

struct thread_data {
	unsigned long id;
	unsigned long run_val_ms;
};

pthread_t thread_array[MAX_TRHEAD];
struct thread_data thread_data_array[MAX_TRHEAD];
static unsigned long cpu_10_ms_count;

static void cpu_10_ms_time(void)
{
	unsigned long cnt = cpu_10_ms_count;
	while(cnt--);
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

void help(void)
{
	printf("--threads   run thread numbers\n");
	printf("--run_cnt   run runtime count\n");
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

	thread_cnt = 1;

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
			{"sleep_cnt",required_argument,	0,	's'},
			{"cpu_util",required_argument,	0,	'c'},
			{0,0,0,0}
		};
	
		int option_index = 0;
		choice = getopt_long( argc, argv, "vht:r:s:l:",
					long_options, &option_index);
	
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
				thread_cnt = atoi(optarg);
				break;
	
			case 'c':
				arg_cpu_util = atoi(optarg);
				if (arg_cpu_util > 100) {
					printf(stderr, " cpu_util must be litter 100\n");
					return -1;
				}
				break;
	
			case '?':
				/* getopt_long will have already printed an error */
				break;
	
			default:
				/* Not sure how to get here... */
				return -1;
		}
	}
	
	for (i = 0; i < thread_cnt; ++i) {
		thread_data_array[i].id = i;
		thread_data_array[i].run_val_ms = 0;
		pthread_create(&thread_array[i], NULL, threadFunc, &thread_data_array[i]);
	}

	for (i = 0; i < thread_cnt; ++i) {
		pthread_join(thread_array[i], NULL);
	}
	return 0;
}
