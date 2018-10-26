#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string>
#include <ctype.h>
#include <fcntl.h>
#include<unistd.h>
#include <sstream>
#include <sys/time.h>
using namespace std;
int main(int argc, char *args[])
{
   char *buf;
   int number;
   if (argc == 1) {
	   printf("not specif the number\n");
	   return 1;
   }

   number=atoi(args[1]);
   while(number--) {
		buf = (char *)malloc(1048567); // alloc 1M buf
		memset(buf, 1, 1048567);
		free(buf);
   }

   while(1)
	   sleep(1);

   return 0;
}

