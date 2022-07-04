#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include<unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/time.h>
#include<semaphore.h>
#include<sched.h>

#define MAX_TRHEAD  (128)

static unsigned long run_val = 1UL<<20;
unsigned long run_cnt;
unsigned long sleep_cnt;
unsigned long thread_cnt;
unsigned long loop_cnt;

struct thread_data {
	unsigned long id;
	unsigned long run_val_ms;
};

pthread_t thread_array[MAX_TRHEAD];
struct thread_data thread_data_array[MAX_TRHEAD];

void *threadFunc(void *param)
{
	struct timeval tvafter,tvpre;
	struct timezone tz;
	unsigned long val = 0;
	struct thread_data *data = (struct thread_data *)param;
	int local_run = run_cnt;
    gettimeofday (&tvpre , &tz);
	while(local_run-->0) {
		int local_loop_cnt = loop_cnt;

		while(local_loop_cnt-->0) {
			val = 0;
			while(val++<run_val);
		}

		if (sleep_cnt)
			usleep(sleep_cnt*1000);

	}
    gettimeofday (&tvafter , &tz);
	data->run_val_ms = (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000;
    printf("花费时间:%ld ms\n", (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);
}

void help(void)
{
	printf("--threads   run thread numbers\n");
	printf("--run_cnt   run runtime count\n");
	printf("--loop_cnt  run loop runtime count\n");
	printf("--sleep_cnt sleep run count\n");
	printf("--sched_policy sleep run count\n");
	printf("--sched_priority sleep run count\n");
}

int main(int argc, char *argv[])
{
	int choice;
	int i, sched_priority = -1, sched_policy = -1;
	struct sched_param param;
	int maxpri;

	thread_cnt = 0;
	run_cnt = 1;
	sleep_cnt = 0;
	loop_cnt = 4;

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
			{"run_cnt", required_argument,	0,	'r'},
			{"sleep_cnt", required_argument,	0,	's'},
			{"loop_cnt", required_argument,	0,	'l'},
			{"sched_policy", required_argument,	0,	'p'},
			{"sched_priority", required_argument,	0,	'i'},
			{ 0, 0, 0, 0 }
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
	
			case 'r':
				run_cnt = atoi(optarg);
				break;

			case 'l':
				loop_cnt = atoi(optarg);
				break;
	
			case 's':
				sleep_cnt = atoi(optarg);
				break;
	
			case '?':
				/* getopt_long will have already printed an error */
				break;
	
			case 'p':
				if (strstr("fifo", optarg))
					sched_priority = SCHED_FIFO;
				else if (strstr("deadline", optarg))
					//sched_priority = SCHED_DEADLINE;
					sched_priority = 6;
				else if (strstr("cpuidle", optarg))
					//sched_priority = SCHED_IDLE;
					sched_priority = 5;
				else if (strstr("qos", optarg))
					sched_priority = 7;
				/* getopt_long will have already printed an error */
				break;

			case 'i':
				sched_priority= atoi(optarg);
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
