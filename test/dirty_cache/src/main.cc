#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include<unistd.h>
#include <sstream>
#include <sys/time.h>
using namespace std;
int main()
{
   char *buf = (char *)malloc(1048567); // alloc 1M buf
   int fd = 0;
   int file_id = 0;
   int i = 0;
   int r = 0;
   int cnt=5;
   string file_name("./testfile");
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
   return 0;
}

