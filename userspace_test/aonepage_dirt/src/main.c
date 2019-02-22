#include<stdio.h>
#include<stdlib.h>
#include <fcntl.h> 
#include <sys/mman.h> 
#include <errno.h> 

#define M1 1024*1024

int main(int argc, char *argv[])
{
	int fd;
	int cache_size, anon_size;
	void *cache_data;
    char read_str[24];

	if (argc != 3)	
		printf("please add cache size and anoe size MB: xx xx\n");

	cache_size = atoi(argv[1]);
	anon_size = atoi(argv[2]);
	printf("zz %s cache_size:%08x anon_size:%08x \n",__func__, (int)cache_size, (int)anon_size);

#if 1
	while(anon_size--) {
		void *addr;
		addr=malloc(M1);
		memset(addr,1,M1);
	}
#endif
	

	cache_data=malloc(M1);
	memset(cache_data,2,M1);

    fd = open("test", O_CREAT | O_RDWR); 
	while(cache_size--) {
		int ret;
		ret=write(fd,cache_data, M1);
	}
    close(fd); 
    printf("input any thinig exit\n");
    scanf("%c", read_str);
	return 0;
}
