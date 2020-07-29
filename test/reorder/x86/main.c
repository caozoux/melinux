#include<semaphore.h>
#include<stdio.h>

#define NULL (0)

sem_t beginSema1;
sem_t beginSema2;
sem_t endSema;

int X, Y;
int r1, r2;

void *thread2Func(void *param)
{
    //MersenneTwister random(1);                // Initialize random number generator
    for (;;)                                  // Loop indefinitely
    {
        sem_wait(&beginSema1);                // Wait for signal from main thread
        while (rand() % 8 != 0) {}  // Add a short, random delay

        // ----- THE TRANSACTION! -----
        X = 1;
        asm volatile("dsb sy" ::: "memory");        // Prevent compiler reordering
	//asm volatile("mfence" ::: "memory");  // Prevent memory reordering
        r1 = Y;
        asm volatile("" ::: "memory");        // Prevent compiler reordering

        sem_post(&endSema);                   // Notify transaction complete
    }
    return NULL;  // Never returns
};

void *thread1Func(void *param)
{
    //MersenneTwister random(1);                // Initialize random number generator
    for (;;)                                  // Loop indefinitely
    {
        sem_wait(&beginSema2);                // Wait for signal from main thread
        while (rand()% 8 != 0) {}  // Add a short, random delay

        // ----- THE TRANSACTION! -----
        Y = 1;
        asm volatile("dsb sy" ::: "memory");        // Prevent compiler reordering
        //asm volatile("" ::: "memory");        // Prevent compiler reordering
	//asm volatile("mfence" ::: "memory");  // Prevent memory reordering
        r2 = X;
        asm volatile("" ::: "memory");        // Prevent compiler reordering

        sem_post(&endSema);                   // Notify transaction complete
    }
    return NULL;  // Never returns
};

int main()
{

	printf("start\n");
    // Initialize the semaphores
    sem_init(&beginSema1, 0, 0);
    sem_init(&beginSema2, 0, 0);
    sem_init(&endSema, 0, 0);

    // Spawn the threads
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread1Func, NULL);
    pthread_create(&thread2, NULL, thread2Func, NULL);

    // Repeat the experiment ad infinitum
    int detected = 0;
    for (int iterations = 1; ; iterations++)
    {
        // Reset X and Y
        X = 0;
        Y = 0;
        // Signal both threads
        sem_post(&beginSema1);
        sem_post(&beginSema2);
        sem_wait(&endSema);
        sem_wait(&endSema);
        // Check if there was a simultaneous reorder
        if (r1 == 0 && r2 == 0)
        {
            detected++;
            printf("%d reorders detected after %d iterations\n", detected, iterations);
        }
        // Wait for both threads
    }
    return 0;  // Never returns
}
