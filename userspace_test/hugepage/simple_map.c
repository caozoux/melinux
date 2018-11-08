#include <fcntl.h> 
#include <sys/mman.h> 
#include <errno.h> 
 
#define MAP_LENGTH      (1000*2*1024*1024) 
 
int main() 
{ 
   int fd; 
   char read_str[24];
   void * addr; 

   /* create a file in hugetlb fs */ 
   fd = open("/mnt/huge/test", O_CREAT | O_RDWR); 
   if(fd < 0){ 
       perror("Err: "); 
       return -1; 
   }   
   printf("fd:%d\n", fd);


   /* map the file into address space of current application process */ 
   addr = mmap(0, MAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
   if(addr == MAP_FAILED){ 
       perror("Err: "); 
       close(fd); 
       unlink("/mnt/huge/test"); 
       return -1; 
   }   

   memset(addr, 1, MAP_LENGTH);
   printf("input any thinig exit\n");
   scanf("%c", read_str);

   /* from now on, you can store application data on huage pages via addr */ 

   munmap(addr, MAP_LENGTH); 
   close(fd); 
   //unlink("/mnt/huge/test"); 
   return 0; 
 }
