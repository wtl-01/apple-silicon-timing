#include "../shared.hh"
#include "../util.hh"
#include "../params.hh"

#define SAMPSIZE (50)

#ifdef TIMING_PTHREAD
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef TIMING_PTHREAD

int terminate = 0;
uint64_t clk = 0;
// Code from SPECTRE: https://github.com/cryptax/spectre-armv7/blob/bc9bd14988d1119242c95042024ff0f20afd2e03/source.c#L165
// Uses dedicated thread that increments once per cycle to act as a timing parameter
// Gururaj said through experience its a good metric to use.

void *thread_function(void *x_void_ptr)
{
    // fprintf(stderr, "[+] Counter thread running...\n");

    while (!terminate)
    {
        // printf("Counter = %llu\n", counter);
        clk++;
    }

    // fprintf(stderr, "[+] Counter thread finished\n");
    return NULL;
}

#endif

uint64_t exp1_tsmp()
{
// Code from SPECTRE: https://github.com/cryptax/spectre-armv7/blob/bc9bd14988d1119242c95042024ff0f20afd2e03/source.c#L165
#ifdef TIMING_PTHREAD
    return clk;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)((uint64_t)(ts.tv_sec) * (uint64_t)(1000 * 1000 * 1000)) + (uint64_t)ts.tv_nsec;
#endif
}

int main(int argc, char **argv)
{

#ifdef TIMING_PTHREAD
    printf("pthread timing active.\n");
    pthread_t cthread;
    // Code from SPECTRE: https://github.com/cryptax/spectre-armv7/blob/bc9bd14988d1119242c95042024ff0f20afd2e03/source.c#L165
    fprintf(stdout, "Creating counter thread\n");
    if (pthread_create(&cthread, NULL, thread_function, NULL))
    {
        fprintf(stdout, "[-] Error creating thread\n");
        perror("pthread");
        return -1;
    }
    fprintf(stdout, "[+] Waiting for thread to start?\n");
    sleep(4);
#endif

    fprintf(stdout, "Experiment-1A, Eviction Set\n");

    for (int i = 0; i < 31; i++)
    {
        uint64_t arrsize = (uint64_t)power(2, i);
        char *arr = (char *)malloc(arrsize);
        if (!arr)
        {
            perror("malloc");
            exit(1);
        }
        char *access = (char *)malloc(sizeof(char));
        if (!access)
        {
            perror("malloc");
            exit(1);
        }
        uint64_t sumtime = 0;
        for (int j = 0; j < SAMPSIZE; j++)
        {
#ifdef TIMING_PTHREAD
            arm_v8_memory_barrier();
            uint64_t t1 = clk;

            (*access);
            arm_v8_memory_barrier();
            uint64_t t2 = clk;
            //fprintf(stdout, "t1, t2, t2-t1: <%llu, %llu, %llu>", t1, t2, (uint64_t)(t2 - t1));
            
            sumtime += (uint64_t)(t2 - t1);
#endif
            for (int k = 0; k < arrsize; k++)
            {
                char x = arr[i];
            }
        }
        double avg_time = (double)(sumtime / (float)SAMPSIZE);
        fprintf(stdout, "%d , %f\n", i, avg_time);

        free(arr);
        free(access);
    }

    fprintf(stdout, "Experiment-1B, DC CIVAC Flush of Cache Line\n");

    char *access2 = (char *)malloc(sizeof(char));
    if (!access2)
    {
        perror("malloc");
        exit(1);
    }
    uint64_t sumtime = 0;
    for (int j = 0; j < SAMPSIZE; j++)
    {
#ifdef TIMING_PTHREAD
        arm_v8_memory_barrier();
        uint64_t t1 = clk;

        (*access2);
        arm_v8_memory_barrier();

        uint64_t t2 = clk;
        fprintf(stdout, "%d , %llu\n", j, (uint64_t)(t2 - t1));
        sumtime += (uint64_t)(t2 - t1);
#endif

        arm_v8_cache_flush((uint64_t)((uintptr_t)access2));
        arm_v8_memory_barrier();
    }
    double avg_time = (double)(sumtime / (float)SAMPSIZE);
    fprintf(stdout, "TOTAL-ITERS, AVG-TIME\n");
    fprintf(stdout, "%d , %f\n", SAMPSIZE, avg_time);
    // Free the requisite memory
    free(access2);

#ifdef TIMING_PTHREAD
    // Exit counter thread
    // Code from SPECTRE.
    terminate = 1;
    if (pthread_join(cthread, NULL))
    {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }
#endif

    return 0;
}