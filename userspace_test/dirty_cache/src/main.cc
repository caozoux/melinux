#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
using namespace std;

static void sync_cache_test(void)
{
   char *buf = (char *)malloc(1048567); // alloc 1M buf
   int fd = 0;
   int file_id = 0;
   int i = 0;
   int r = 0;
   int cnt=5;
   string file_name("/tmp/testfile");
   struct timeval tvafter,tvpre;
   struct timezone tz;
   while (cnt>0) {
     file_id ++;
     stringstream ss;
     ss << file_id;
     string sub_filename = file_name + ss.str();
     fd = open(sub_filename.c_str(), O_RDWR|O_CREAT, 0640);
     printf("fd:%d\n", fd);
     for (i = 0; i < 500; i++) {
       r = write(fd, buf, 1048676);
       //usleep(30000);
       usleep(80000);
     }
     printf("sync +\n");
     gettimeofday (&tvpre , &tz);
     fsync(fd);
     gettimeofday (&tvafter , &tz);
     printf("花费时间:%d\n", (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);
     printf("sync -\n");
     close(fd);
     cnt--;
     printf("cnt%d\n",cnt);
   }
}

static int mmap_cache_test(void)
{
	void *addr = 0;
	int fd,i;
	fd = open("/tmp/mmapdata", O_RDWR);
	if (fd <= 0) {
		printf("open /tmp/mmapdata failed\n");
		return -1;
	}
	addr = mmap(NULL, 0x10000000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	printf("zz %s addr:%lx \n", __func__, (unsigned long)addr);
	for (i = 0; i < 10; i++) {
		printf("zz %s addr:%lx \n",__func__, (unsigned long)addr);
		memset(addr, 0, 0x100000);
		addr +=0x100000;
	}
	printf("zz %s %d \n", __func__, __LINE__);
	sleep(100);
	return 0;
}

static struct option long_options[] =
{
	/* Use flags like so:
	{"verbose",	no_argument,	&verbose_flag, 'V'}*/
	/* Argument styles: no_argument, required_argument, optional_argument */
	{"version", no_argument,	0,	'v'},
	{"help",	no_argument,	0,	'h'},
	{"size", required_argument,	0,	's'},
	{"file", required_argument,	0,	'f'},
	{"testcase", required_argument,	0,	't'},
	{ 0, 0, 0, 0 }
};

static char *tastcaselist[] =
{
"mmap",
"read",
"write",
};

void run_case(char run_case)
{
	int i;
	if (strstr(run_case,"mmap")) {
		
	} else if (strstr(run_case,"mmap")) {

	} else if () {

	} else {

	}
	
	return ;
}

int main()
{
	int option_index = 0;
	int args_size = 0;
	int args_filename[256] = {0};
	int args_case[64] = {0};
	choice = getopt_long( argc, argv, "vht:r:s:l:",
				long_options, &option_index);

    fd = open("/mnt/test/44", O_RDWR, 0640);
	while(1)
		sleep(1);
	if (choice == -1)
		break;

	switch( choice )
	{
		case 'v':
			break;

			case 'h':
				help();

			case 'h':
				break;

			case 's':
				thread_cnt = atoi(optarg);
				break;

			case 'f':
				strncpy(args_filename, optarg, 256);
				break;

			case 't':
				strncpy(args_case, optarg, 256);
				break;

			default:
				break;
	}
	
	run_case(args_case);
			
	//mmap_cache_test();
	return 0;
}

