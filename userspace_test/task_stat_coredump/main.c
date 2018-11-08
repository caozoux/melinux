#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
	setrlimit(RLIMIT_CORE, &(struct rlimit){});

	while (1) {
		char buf[64];
		char buf2[4096];
		pid_t pid;
		int fd;

		pid = fork();
		if (pid == 0) {
			*(volatile int *)0 = 0;
		}

		snprintf(buf, sizeof(buf), "/proc/%u/stat", pid);
		fd = open(buf, O_RDONLY);
		read(fd, buf2, sizeof(buf2));
		close(fd);

		waitpid(pid, NULL, 0);
		printf("pid:%d\n", pid);
	}
	return 0;
}
