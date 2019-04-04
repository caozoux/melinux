#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

enum SERVERTYPE{COMMON, STANDALONE, MULTITHREAD, POOL, QUEUE, GSI};
int keepalive = 0, dime = 0, chunk = 0;
#define  LOGBUFSZ 1024

static void domem_test(int size)
{
	void *buf;
	while(size--) {
		buf = malloc(1024*1024);
		if (!buf) {
			printf("zz %s %d alloc mem failed\n", __func__, __LINE__);
			exit(1);
		}
		memset(buf, 0, 1024*1024);
	}

}
/* WSIO server main function */
int main(int argc, char **argv)
{
	int c;
	int sleep_en = 0;
	char host[128] = "localhost";
	char log_buf[LOGBUFSZ];
	int port = 8080, backlog = 100;
	enum SERVERTYPE servertype = COMMON;
	int memsize=0;
	int helpflg = 0,
	errflg = 0,
	debug = 0;
	struct option longopts[] =
	{
		{"host", 1, 0, 'h'},
		{"port", 1, 0, 'p'},
		{"backlog", 1, 0, 'b'},
		{"type", 1, 0, 't'},
		{"keepalive", 0, 0, 'k'},
		{"chunk", 0, 0, 'c'},
		{"dime", 0, 0, 'd'},
		{"debug", 0, 0, 'D'},
		{"mem", 1, 0, 'm'},
		{"sleep", 1, 0, 's'},
		{"help", 0, &helpflg, 1},
		{0, 0, 0, 0}
	};

	while ((c = getopt_long (argc, argv, "h:p:b:t:kcdDm:s", longopts, NULL)) != EOF)
	{
		switch(c)
		{
		case 'm':
			memsize = atoi(optarg);
			printf("alloc memory size:%d MB\n",  memsize);
			domem_test(memsize);
			break;
		case 'h':
			sprintf(host, "%s", optarg);
			break;
		case 'p':
			port = atoi(optarg);
			printf("zz %s port:%08x \n",__func__, (int)port);
			break;
		case 'b':
			backlog = atoi(optarg);
			break;
		case 't':
			switch(*optarg)
			{
			case 'C':
				servertype = COMMON;
				break;
			case 'S':
				servertype = STANDALONE;
				break;
			case 'M':
				servertype = MULTITHREAD;
				break;
			case 'P':
				servertype = POOL;
				break;
			case 'Q':
				servertype = QUEUE;
				break;
			case 'G':
				servertype = GSI;
			default:
				break;
			}
			break;
		case 'k':
			keepalive = 1;
			break;
		case 'c':
			chunk = 1;
			break;
		case 'd':
			dime = 1;
			break;
		case 'D':
			debug = 1;
			break;
		case 's':
			sleep_en = 1;
			break;
		case '?':
			errflg++;
			break;
		default:
			break;
		}
	}

	if(helpflg || errflg)
	{
		fprintf(stderr,"Usage: wsiod [OPTION]\n\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		"WSIO server based on web service.\n",
		"Mandatory arguments to long options are mandatory for short options too.\n",
		"/t-h, --host hostname in soap_bind, default is host which the service runs\n ",
		"/t-p, --port port which the sercer runs on, default is 8080\n",
		"/t-b, --backlog request backlog, default is 100\n",
		"/t-t, --type server type, default is COMMON\n",
		"/t-k, --keepalive attempt to keep socket connections alive\n",
		"/t-c, --chunk use HTTP chunking\n",
		"/t-d, --dime use DIME encoding\n",
		"/t-D, --debug print debug info\n",
		"/t --help print this help\n\n",
		"Server type:\n",
		"/tCOMMON the simplest server\n",
		"/tSTANDALONE stand-alone server, which can run on port 80\n"
		"/tMULTITHREAD multi-thread stand-alone server\n",
		"/tPOOL using a pool of servers\n",
		"/tQUEUE using a queue of requests for server\n",
		"/tGSI prethreaded server with GSI enabled\n\n",
		"Report bugs to <Aigui.LIU@ihep.ac.cn>.\n"
		);
		exit(0);
	}

	if (sleep_en) {
		while(1)
			sleep(1);
	}
	/* 省略部分 */

	return 0;
}

