#include <semaphore.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

int add(int a, int b)
{
    return a + b;
}

int new_func(int a, int b)
{
	return 3;
} 

void sig_user1_handler(int sig, siginfo_t *si, void *data)
{
    int pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize < 0) {
        pagesize = 4096;
    }

    int len = sizeof(new_func);

    uintptr_t addr = (((uintptr_t)add) / pagesize) * pagesize;
    fprintf(stderr, "%s: iminus: %p, aligned: 0x%lx, sz %d\n", __func__, add, addr, len);
    if (mprotect((void*)addr, (uintptr_t)add - addr + len, PROT_WRITE|PROT_READ|PROT_EXEC) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
    }

    memcpy((void*)add, (void*)new_func, len);

    if (mprotect((void*)addr, (uintptr_t)add - addr + len, PROT_READ|PROT_EXEC) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
    }
}


int main()
{
    struct sigaction newact, oldact;
    sigemptyset(&newact.sa_mask);
    newact.sa_sigaction = sig_user1_handler;
    sigaction(SIGUSR1, &newact, &oldact);

    int pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize < 0) {
        pagesize = 4096;
    }
    int len = sizeof(new_func);

    uintptr_t addr = (((uintptr_t)add) / pagesize) * pagesize;
    fprintf(stderr, "%s: iminus: %p, aligned: 0x%lx, sz %d\n", __func__, add, addr, len);
    if (mprotect((void*)addr, (uintptr_t)add - addr + len, PROT_WRITE|PROT_READ|PROT_EXEC) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
	return;
    }

    memcpy((void*)add, (void*)new_func, 32);

    if (mprotect((void*)addr, (uintptr_t)add - addr + len, PROT_READ|PROT_EXEC) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
    }
    for(;;)
    {
        printf("%d\n", add(1, 1));
        sleep(3);
    }
}

