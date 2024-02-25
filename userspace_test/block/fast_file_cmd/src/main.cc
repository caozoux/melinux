#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include<unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<fcntl.h>
#include<semaphore.h>
#include<sys/types.h>
#include<string>
#include<sstream>
#include<libgen.h>
#include<sys/mman.h>

#include<linux/types.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/stat.h>



using namespace std;

#define MAP_LENGTH (1UL<<32)

#define MAX_TRHEAD  (1)
enum  OP_TYPE{
	OP_TYPE_NODE,
	OP_TYPE_CREATE_FILE,
	OP_TYPE_DEL_FILE,
	OP_TYPE_CREATE_DIR,
	OP_TYPE_DEL_DIR,
	OP_TYPE_LINK,
	OP_TYPE_UNLINK,
	OP_TYPE_MMAP,
};


unsigned long run_cnt;
unsigned long sleep_cnt=0;
unsigned long thread_cnt;
unsigned long loop_cnt;
unsigned long msec_cnt;
int sched_policy = 0;

struct thread_data {
	string dirname;
	enum OP_TYPE type;
};

typedef void *(*Thread_func)(void *param);
pthread_t thread_array[MAX_TRHEAD];

void *op_mmap_thread(void *param)
{
	int fd;
    int file_id = 0;
	unsigned long size = MAP_LENGTH;
	struct thread_data *data = (struct thread_data *)param;
    string file_name = data->dirname;
 	string sub_filename = file_name;
	char *buf;
	unsigned long offset = 0;

	fd = open(sub_filename.c_str(), O_RDWR|O_CREAT, 0640);
	if (fd < 0)
		goto out;

	buf = (char *)mmap(0, MAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	printf("zz %s buf:%lx \n",__func__, (unsigned long)buf);
	while(size > offset)
	{
		buf[offset]	= 0;
		offset += 512;
	}

	close(fd);
out:
	return NULL;
}

void *threadFunc(void *param)
{
	struct timeval tvafter,tvpre;
	struct timezone tz;
    int cnt=1000;
    int file_id = 0;
	unsigned long val = 0;
	struct thread_data *data = (struct thread_data *)param;
    //string file_name = data->dirname;
    string file_name = data->dirname;

	printf("zz %s %d \n", __func__, __LINE__);
    gettimeofday (&tvpre , &tz);

    while (cnt--) {
		int fd;
		stringstream ss;
		ss << file_id;
 		string sub_filename = file_name+"/"+ ss.str();
		if (data->type == OP_TYPE_CREATE_FILE) {
			fd = open(sub_filename.c_str(), O_RDWR|O_CREAT, 0640);
			if (fd < 0)
				break;
			close(fd);
		} else if (data->type == OP_TYPE_CREATE_DIR) {
			//mkdir(sub_filename.c_str(), 0640);
		}
		
		file_id++;
	}

    gettimeofday(&tvafter , &tz);
    printf("花费时间:%ld ms\n", (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);
	return NULL;
}

void help(void)
{
	printf("--threads/t   run thread numbers\n");
	printf("--type/m  	  create/rmdir\n");
	printf("-f  		  -s 1K -f data\n");
}

static void testcase_create(Thread_func func)
{
	struct thread_data thread_data_array[MAX_TRHEAD];
	int i;
	for (i = 0; i < MAX_TRHEAD; i++) {
		stringstream ss;
		ss << i;
 		string sub_filename = "test"+ ss.str();
		thread_data_array[i].dirname= sub_filename;
		//thread_data_array[i].dirname= "test";
		pthread_create(&thread_array[i], NULL, func, &thread_data_array[i]);
	}

	for (i = 0; i < MAX_TRHEAD; i++) {
		pthread_join(thread_array[i], NULL);
	}
}

/******************************************************************************
* Function:         static unsigned long tran_size
* Description:      
* Where:
* Return:           
* Error:            
*****************************************************************************/
static unsigned long tran_size(const char *str)
{
	int len = strlen(str);
	unsigned long unit_size;
	char lo_str[256] = {0};

	char s_str = str[len-1];

	switch (s_str) {
		case 'k':
			unit_size = 1024;
			break;
		case 'K':
			unit_size = 1024;
			break;
		case 'm':
			unit_size = 0x100000;
			break;
		case 'M':
			unit_size = 0x100000;
			break;
		case 'g':
			unit_size = 1<<30;
			break;
		case 'G':
			unit_size = 1<<30;
			break;
		default:
			return -1;
	}
	strncpy(lo_str, str, len-1);
	
	return atoi(lo_str) * unit_size;
}

/******************************************************************************
* Function:         static int create_file_size
* Description:      
* Where:
* Return:           
* Error:            
*****************************************************************************/
static int create_file_size(const char *filename, unsigned long size)
{
	int fd;
	unsigned long buf[64], i;
	unsigned long sector = size/512;

	memset(buf, 0, 512);
	fd = open(filename , O_RDWR|O_CREAT | O_LARGEFILE, 0640);
	if (fd <= 0) {
		perror("open fialed");
		return -1;
	}

	for (i = 0; i < sector; ++i) {
		buf[0] = i;
		write(fd, buf, 512);
	}

	close(fd);

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;
	int i;
	enum OP_TYPE op_type;
	static unsigned long args_size = 0;
	static char *args_filename = NULL;

	thread_cnt = 0;

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
			{"help",	no_argument,	0,	'h'},
			{"threads",	required_argument,	0,	't'},
			{"size",	required_argument,	0,	's'},
			{"mode", required_argument,	0,	'p'},
			{"name", required_argument,	0,	'f'},
			{ 0, 0, 0, 0 }
		};
	
		int option_index = 0;
		choice = getopt_long(argc, argv, "ht:m:s:f:",
					long_options, &option_index);
	
		if (choice == -1)
			break;
	
		switch( choice )
		{
			case 'h':
				help();
				return 0;

			case 's':
				args_size = tran_size(optarg);
				break;

			case 'f':
				args_filename = (char *)malloc(strlen(optarg));
				args_filename= strcpy(args_filename, optarg);
				break;
	
			case 't':
				thread_cnt = atoi(optarg);
				break;
	
			case 'm':
				if (strstr("cr_file", optarg))
					op_type = OP_TYPE_CREATE_FILE;
				else if (strstr("del_file", optarg))
					op_type = OP_TYPE_DEL_FILE;
				else if (strstr("cr_dir", optarg))
					op_type = OP_TYPE_CREATE_DIR;
				else if (strstr("del_dir", optarg))
					op_type = OP_TYPE_DEL_DIR;
				else if (strstr("link", optarg))
					op_type = OP_TYPE_LINK;
				else if (strstr("unlink", optarg))
					op_type = OP_TYPE_UNLINK;
				else if (strstr("mmap", optarg))
					op_type = OP_TYPE_MMAP;
				else {
					printf("sched policy not support\n");
					exit(1);
				}
				/* getopt_long will have already printed an error */
				break;

			default:
				/* Not sure how to get here... */
				return -1;
		}
	}

	switch (op_type) {
		case OP_TYPE_CREATE_FILE:
			break;
		case OP_TYPE_MMAP:
			testcase_create(op_mmap_thread);
			break;
		default:
			break;
	}

	if (args_filename && args_size > 0)
		create_file_size(args_filename, args_size);

	return 0;
}

