#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <sys/time.h>
using namespace std;

void write_file()
{

   int r = 0;
   int i = 0;
   char *buf = (char *)malloc(1048567); // alloc 1M buf
   int fd = 0;
   int file_id = 0;
   int cnt=5;
   string file_name("./testfile");
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
     fsync(fd);
     close(fd);
     cnt--;
   }
}

void vmalloc_test(void)
{
	void *buf;
	int size=1024*1024*512;
	buf = malloc(size);
	printf("zz %s buf:%p \n",__func__, buf);
	memset(buf, 1, size);

}

int main()
{
   struct timeval tvafter,tvpre;
   struct timezone tz;
   gettimeofday (&tvpre , &tz);
   printf("花费时间:%d\n", (tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);
   vmalloc_test();
   while(1)
	   sleep(1);
   return 0;
}

