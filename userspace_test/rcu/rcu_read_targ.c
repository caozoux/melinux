#include <fcntl.h> 
#include <sys/mman.h> 
#include <errno.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
 

static int rcu_read(void)
{
	int fd;

    fd = open("/dev/rcu_hood", O_RDWR);
    if(fd < 0){ 
       perror("Err: dev"); 
       return -1; 
    }   

	while(1) {
		ioctl(fd, 1, 0);
		ioctl(fd, 2, 0);
	}
	close(fd);

	return 0;
}


int main(int argc, char *argv[])
{
	rcu_read();
	return 0;
}
