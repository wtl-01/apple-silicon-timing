#include "../shared.hh"
#include "../util.hh"
#include "../params.hh"

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

uint64_t timestamp()
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

uint64_t measure_latency_two_access(uint64_t addr_A, uint64_t addr_B)
{

#ifdef TIMING_PTHREAD
  printf("pthread timing active \n");
  pthread_t cthread;
  // Code from SPECTRE: https://github.com/cryptax/spectre-armv7/blob/bc9bd14988d1119242c95042024ff0f20afd2e03/source.c#L165
  fprintf(stderr, "Creating counter thread\n");
  if (pthread_create(&cthread, NULL, thread_function, NULL))
  {
    fprintf(stderr, "[-] Error creating thread\n");
    perror("pthread");
    return -1;
  }
  fprintf(stderr, "[+] Waiting for thread to start?\n");
  // sleep(1);
#endif
  // run clflush2(addr_A);
  // run clflush2(addr_B);
  arm_v8_cache_flush(addr_A);
  arm_v8_cache_flush(addr_B);

  uint8_t *addr_A_ptr = reinterpret_cast<uint8_t *>(addr_A);
  uint8_t *addr_B_ptr = reinterpret_cast<uint8_t *>(addr_B);

  // Deleted part where Shubh first LDR's then evicts an address from cache
  // (*addr_A_ptr);
  // run clflush2(addr_A);
  // arm_v8_cache_flush(addr_A);

  // Credit: IAIK - ARMageddon Paper
  // lfence();
  arm_v8_memory_barrier();
  
  uint64_t t1 = 0;
  uint64_t t2 = 0;
  // Two ways of measuring timestamps: timestruct vs kernel of rdtscp
#ifdef TIMING_PTHREAD
  t1 = clk;
#endif
  // uint64_t start = rdtsc();

  (*addr_A_ptr);
  (*addr_B_ptr);

  // lfence();
  arm_v8_memory_barrier();
#ifdef TIMING_PTHREAD
  t2 = clk;
#endif
  // uint64_t end = rdtsc();

#ifdef TIMING_PTHREAD
  // Exit counter thread
  // Code from SPECTRE.
  terminate = 1;
  if (pthread_join(cthread, NULL))
  {
    fprintf(stderr, "Error joining thread\n");
    perror("join");
    exit(1);
  }
#endif

  return (uint64_t)t2 - t1;
  // return (uint64_t) end - start;
}


int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    uint64_t buffer_size_bytes = (uint64_t) BUFFER_SIZE_MB * (1024*1024);
    allocated_mem = allocate_pages(buffer_size_bytes);
    uint64_t* bank_lat_histogram = (uint64_t*) calloc((100+1), sizeof(uint64_t));
    
    const long int num_iterations = buffer_size_bytes / ROW_SIZE;
    char *base = (char *)allocated_mem;
    for (int i = 1; i < num_iterations; i++) {
        uint64_t time = 0;
        for (int j = 0; j < SAMPLES; j++) {
            time += measure_latency_two_access((uint64_t)base, (uint64_t)(base + i * ROW_SIZE));
        }
        double avg_time = (double) ( time / (float) SAMPLES);
        bank_lat_histogram[(int) (avg_time / 10)]++;
    }

    //Modify Shubh's format
    puts("HEADER,HEADER");
    printf("Total Number of pairs, %ld\n", num_iterations);
  
    puts("TABLESTART,TABLESTART");
    printf("UNIT,NS\n");
    printf("TIMING-METHOD,POSIX\n");
    printf("Timing-Unit,Number-of-Address-Pairs\n");
    

    for (int i=0; i<100 ;i++){
        printf("[%d-%d),%15llu\n",
	    i*10, i*10 + 10, bank_lat_histogram[i]);
    }
    printf("[%d),%15llu \n",100*10, bank_lat_histogram[100]);
}