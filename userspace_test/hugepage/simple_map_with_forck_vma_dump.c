#include <fcntl.h> 
#include <sys/mman.h> 
#include <errno.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
 
#define MAP_LENGTH      (10*2*1024*1024) 
 
static void child_process(unsigned long addr)
{
	int fd;


    printf("zz %s fpid:%lx \n",__func__, addr);
    fd = open("/dev/vma_dump", O_RDWR);
    if(fd < 0){ 
       perror("Err: dev"); 
       return -1; 
    }   
    memset(addr, 2, MAP_LENGTH);
    ioctl(fd, 1, addr);
	close(fd);

}

int main() 
{ 
   int fd; 
   int fpid;	
   int ret, status;
   char read_str[24];
   void * addr; 

   /* create a file in hugetlb fs */ 
   fd = open("/mnt/huge/test", O_CREAT | O_RDWR); 
   if(fd < 0){ 
       perror("Err: "); 
       return -1; 
   }   


   /* map the file into address space of current application process */ 
   addr = mmap(0, MAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
   if(addr == MAP_FAILED){ 
       perror("Err: "); 
       close(fd); 
       unlink("/mnt/huge/test"); 
       return -1; 
   }   

   memset(addr, 1, MAP_LENGTH);

   fpid=fork();
   if (fpid < 0)   
        printf("error in fork!");   
   else if (fpid == 0) {  
        printf("child process, is %d/n",getpid());   
		child_process(addr);
		while(1) {
			sleep(1);
		}
   } else {
	   printf("zz %s fpid:%d \n",__func__, (int)fpid);
	   printf("input any thinig exit\n");
	   scanf("%c", read_str);
	   //kill(fpid, SIGKILL);
	   //ret = waitpid(fpid, &status, WNOHANG);
	   //printf("zz %s ret:%08x \n",__func__, (int)ret);
	 

	   /* from now on, you can store application data on huage pages via addr */ 

	   munmap(addr, MAP_LENGTH); 
	   close(fd); 
	   //unlink("/mnt/huge/test"); 
	   return 0; 
   }
return 0; 
 }
