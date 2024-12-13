#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#ifndef __UTILITY_H__
#define __UTILITY_H__

#define SAMPLES 10

// Function to read the time stamp counter, which is called tsc for short
// "rdtscp" returns a 32bit unsigned integer
// "rdtscp64" return a 64 bit unsigned integer
// Details in https://www.felixcloutier.com/x86/rdtscp
// static inline uint32_t rdtscp() {
//   uint32_t rv;
//   asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
//   return rv;
// }

// static inline uint64_t rdtscp64() {
//   uint32_t low, high;
//   asm volatile ("rdtscp": "=a" (low), "=d" (high) :: "ecx");
//   return (((uint64_t)high) << 32) | low;
// }

// Function "lfence" wrap the assembly instruction lfence
// This function performs a serializing operation which ensures that
// the instructions after "lfence" start execution after
// all the instructions before "lfence" complete
// Details in https://www.felixcloutier.com/x86/lfence
// static inline void lfence() {
//   asm volatile("lfence");
// }

static inline void arm_v8_memory_barrier(void)
{
  asm volatile("DSB SY");
  asm volatile("ISB");
}

static inline void arm_v8_dsb_barrier(void)
{
  asm volatile("DSB SY");
}

static inline int power(int base, unsigned int exp)
{
  int i, result = 1;
  for (i = 0; i < exp; i++)
    result *= base;
  return result;
}

// Here is an example of using "rdtsc" and "lfence" to
// measure the time it takes to access a block specified by its virtual address
// The corresponding pseudo code is
// =========
// t1 = rdtsc
// load addr
// t2 = rdtsc
// cycles = t2 - t1
// return cycles
// =========
// static inline uint64_t measure_one_block_access_time(uint64_t addr)
// {
// 	uint64_t cycles;

// 	asm volatile("mov %1, %%r8\n\t"
// 	"lfence\n\t"
// 	"rdtsc\n\t"
// 	"mov %%eax, %%edi\n\t"
// 	"mov (%%r8), %%r8\n\t"
// 	"lfence\n\t"
// 	"rdtsc\n\t"
// 	"sub %%edi, %%eax\n\t"
// 	: "=a"(cycles) /*output*/
// 	: "r"(addr)    /*input*/
// 	: "r8", "edi");	/*reserved register*/

// 	return cycles;
// }

static inline void arm_v8_cache_flush(uint64_t addr)
{
  // asm volatile ("":::);
  //  Anything XX X(I)VAC should work.
  //  asm volatile ("IC IVAU, %0" :: "r"(addr));
  //  The above should not work.

  asm volatile("DC CIVAC, %0" ::"r"(addr));
  // asm volatile ("DC CVAC, %0" :: "r"(addr));
  asm volatile ("IC IVAU, %0" :: "r"(addr));

  // Some combination of memory barriers needed.
  asm volatile("DSB SY");
  asm volatile("ISB");
}


// memory load
/**
 * Using STR VAL, [ADDR] to store a value into an address.
*/
static inline void store_value(int* addr, int value) {
    asm volatile ("str %1, [%0]" :: "r" (addr), "r" (value) );
}

/**
 * Sourced from:
 * https://lore.kernel.org/lkml/20200914115311.2201-3-leo.yan@linaro.org/
 */
static inline uint64_t rdtsc(void)
{
  uint64_t val;

  /*
   * According to ARM DDI 0487F.c, from Armv8.0 to Armv8.5 inclusive, the
   * system counter is at least 56 bits wide; from Armv8.6, the counter
   * must be 64 bits wide.  So the system counter could be less than 64
   * bits wide and it is attributed with the flag 'cap_user_time_short'
   * is true.
   */
  asm volatile("mrs %0, cntvct_el0" : "=r"(val));

  return val;
}

// static inline void one_block_access(uint64_t addr)
// {
// 	asm volatile("mov (%0), %%r8\n\t"
// 	: /*output*/
// 	: "r"(addr)    /*input*/
// 	: "r8");	/*reserved register*/

// }

// static inline uint64_t two_maccess_t(uint64_t addr_a, uint64_t addr_b)
// {
// 	uint64_t cycles;

// 	asm volatile(
// 		"mov %1, %%r8\n\t"
// 		"mov %2, %%r9\n\t"
// 		"lfence\n\t"
// 		"rdtsc\n\t"
// 		"mov %%eax, %%edi\n\t"
// 		"mov (%%r8), %%r8\n\t"
// 		"mov (%%r9), %%r9\n\t"
// 		"lfence\n\t"
// 		"rdtsc\n\t"
// 		"sub %%edi, %%eax\n\t"
// 		: "=a"(cycles) /*output*/
// 		: "r"(addr_a), "r"(addr_b)    /*input*/
// 		: "r8", "r9", "edi"
// 	);	/*reserved register*/

// 	return cycles;
// }

// A wrapper function of the clflush instruction
// The instruction evict the given address from the cache to DRAM
// so that the next time the line is accessed, it will be fetched from DRAM
// Details in https://www.felixcloutier.com/x86/clflush
// static inline void clflush(void *v) {
//   asm volatile ("clflush 0(%0)": : "r" (v):);
// }

// static inline void clflush2(uint64_t addr)
// {
//   asm volatile ("clflush (%0)"
// 		: /*output*/
// 		: /*input*/ "r"(addr)
// 		: /*clobbers*/ );
// }

// /* Ensure all instructions execute before before anything else can exec. */
// static inline void mfence()
// {
//   asm volatile ("mfence"
// 		: /*output*/
// 		: /*input*/
// 		: /*clobbers*/ );
// }

#endif // _UTILITY_H__
