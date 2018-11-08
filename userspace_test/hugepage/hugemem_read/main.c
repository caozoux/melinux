#include <fcntl.h> 
#include <sys/mman.h> 
#include <errno.h> 
 
#define MAP_LENGTH      (5*2*1024*1024) 
 
int main() 
{ 
   int fd; 
   char read_str[24];
   char buf[512];
   void * addr; 

   /* create a file in hugetlb fs */ 
   fd = open("/mnt/huge/test", O_CREAT | O_RDWR); 
   if(fd < 0){ 
       perror("Err: "); 
       return -1; 
   }   
   printf("fd:%d\n", fd);
   read(fd, buf, 512);


   printf("input any thinig exit\n");
   scanf("%c", read_str);

   /* from now on, you can store application data on huage pages via addr */ 
   close(fd); 
   //unlink("/mnt/huge/test"); 
   return 0; 
 }
