#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h> 
#include <arpa/inet.h>
#include <sys/mman.h> 
#include <signal.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <sstream>
#include <string>
#include <vector>
#include <list>

using namespace std;

unsigned int port_to_32bit(unsigned short int sport, unsigned short int dport)
{
	unsigned int rval=0;
	unsigned char val;
	unsigned char *p;

	p=(unsigned char *)&dport;

	rval += p[1];
	rval += p[0]<<8;

	p=(unsigned char *)&sport;

	rval += p[1]<<16;
	rval += p[0]<<24;

	return rval;
}

int caculate_hash(unsigned int src, unsigned int sport, unsigned int dst, unsigned dport)
{
   unsigned int hash = port_to_32bit(sport, dport) ;
   hash ^= src ^ dst;
   hash ^= (hash >> 16);                                                                                                                                                                                                                            
   hash ^= (hash >> 8);

  return hash >> 1;
}


unsigned int ip_to_32bit(const char *ip_addr)
{
    unsigned int ip = 0;
    char *p, *q;
	unsigned int result;
	int i = 0;

    p = (char*)ip_addr;
    q = strtok(p, ".");
    while (q != NULL) {
        //ip = ip << 8;
        ip += atoi(q) << (8*i);
        q = strtok(NULL, ".");
		i++;
    }
    sprintf((char*)&result, "%x", ip);
    return ip;
}

int main(int argc, char *argv[])
{
	int args_size = 0;
	int choice;
	int fd;
    char srcip[16], dstip[16];
	unsigned short int src_port, dst_port;
	unsigned int src_ip, dst_ip;
	unsigned int ret;

	while (1) {
		int option_index = 0;
		choice = getopt_long( argc, argv, "vhs:p:d:e:",
					NULL, &option_index);
		if (choice == -1)
			break;
		switch( choice )
		{
			case 'v':
				break;
	
			case 'h':
				return 0;
	
			case 's':
				memcpy(srcip, optarg, strlen(optarg));
				src_ip=ip_to_32bit(srcip);
				break;

			case 'p':
				src_port = atoi(optarg);
				break;

			case 'd':
				memcpy(dstip, optarg, 16);
				dst_ip=ip_to_32bit(dstip);
				//memcpy(args_file, optarg, strlen(optarg));
				break;

			case 'e':
				dst_port = atoi(optarg);
				//args_size = atoi(optarg);
				break;


			default:
				return -1;
		}
	}
	ret = caculate_hash(src_ip, src_port, dst_ip, dst_port);
	printf("zz %s ret:%lx \n",__func__, (unsigned long)ret);
	return 0;
}

