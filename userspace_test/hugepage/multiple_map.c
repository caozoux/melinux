#include <fcntl.h> 
#include <sys/mman.h> 
#include <errno.h> 
 
#define MAP_LENGTH      (10*2*1024*1024) 
 
static void multi_map(int fd)
{
	int count=1208,i;
	unsigned long start_vir_addr=0x30000000;

	for (i = 0; i < count; ++i) {
		void *map_addr;
   		map_addr = mmap(start_vir_addr+(i*MAP_LENGTH), MAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
   		if(map_addr == MAP_FAILED){ 
       		perror("Err: "); 
		}
   		memset(map_addr, 2, MAP_LENGTH);
		printf("zz %s map_addr:%08x \n",__func__, (int)map_addr);
	}
}

int main() 
{ 
   int fd; 
   char read_str[24];
   void * addr; 

   /* create a file in hugetlb fs */ 
   fd = open("/mnt/huge/test", O_RDWR); 
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
   memset(addr, 2, MAP_LENGTH);
   multi_map(fd);
   printf("input any thinig exit\n");
   scanf("%c", read_str);

   /* from now on, you can store application data on huage pages via addr */ 

   munmap(addr, MAP_LENGTH); 
   close(fd); 
   //unlink("/mnt/huge/test"); 
   return 0; 
 }
