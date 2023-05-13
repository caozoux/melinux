#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include<unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/time.h>
#include<semaphore.h>
#include<linux/sched.h>

#define MAX_TRHEAD  (128)

unsigned long run_cnt;
unsigned long sleep_cnt=0;
unsigned long thread_cnt;
unsigned long loop_cnt;
unsigned long msec_cnt;
int sched_policy = 0;

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
	int policy = SCHED_NORMAL;
	struct sched_param my_params;
	int rc, local_loop_cnt = loop_cnt;

    gettimeofday (&tvpre , &tz);
	//my_params.sched_priority=sched_get_priority_max(sched_policy);// 尽可能高的实时优先级
	my_params.sched_priority=sched_get_priority_min(sched_policy);// 尽可能高的实时优先级

	if (sched_policy) {
		//rc = sched_setscheduler(0,sched_policy, &my_params);
		rc = sched_setscheduler(0,sched_policy, &my_params);
		printf("set sched policy:%d priority:%d\n", sched_policy, my_params.sched_priority);
		if(rc<0)
		{
			//perror("sched_setscheduler to %d error", sched_policy);
			perror("sched_setscheduler error");
			exit(0);
		}
	}

	while(local_loop_cnt-->0) {
		val = 0;
		while(val++<msec_cnt);
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
	printf("--sched_policy fifo/deadline/cpuidle/qos\n");
	printf("--sched_priority  sched priority vaule -20 - 120\n");
}

static int check_cpu_runtime(void)
{
	struct timeval tvafter,tvpre;
	struct timezone tz;
	unsigned long usec;

	unsigned long cnt = 1UL<<30;
    gettimeofday (&tvpre , &tz);

	while(cnt-->0);
    gettimeofday (&tvafter , &tz);

	usec= (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000;
	msec_cnt = (1UL<<30)/usec;
    printf("花费时间:%ld ms %ld\n", usec, msec_cnt);
}

int main(int argc, char *argv[])
{
	int choice;
	int i, sched_priority = -1;
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
		choice = getopt_long(argc, argv, "vht:r:s:l:p:",
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
	
			case 'p':
				if (strstr("fifo", optarg))
					sched_policy = SCHED_FIFO;
				else if (strstr("deadline", optarg))
					sched_priority = SCHED_DEADLINE;
				else if (strstr("cpuidle", optarg))
					//sched_priority = SCHED_IDLE;
					sched_policy = SCHED_IDLE;
				else if (strstr("rr", optarg))
   					sched_policy = SCHED_RR;
				else if (strstr("qos", optarg))
					sched_priority = 7;
				else {
					printf("sched policy not support\n");
					exit(1);
				}
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
	
	check_cpu_runtime();
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

