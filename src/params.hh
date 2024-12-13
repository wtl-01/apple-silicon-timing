#ifndef PARAMS_GUARD
#define PARAMS_GUARD

// Run This number of hammering operations, hammering across all rows, per iteration
#ifndef NUM_HAMMER_ITERS
#define NUM_HAMMER_ITERS 5000000
#endif

// Size of allocated buffer
#ifndef BUFFER_SIZE_MB
#define BUFFER_SIZE_MB 2048ULL
#endif

// IRRELEVANT PARAMETERS FROM x86 EXPERIMENTS:

// Size of hugepages in system
#define HUGE_PAGE_SIZE (1 << 21)

// Size of regular pages in the system
#define PAGE_SIZE (1 << 12)

// Number of offset bits
#define PAGE_OFFSET_BITS 12

// Size of DRAM row in bytes (1 bank)
#define ROW_SIZE (8192)

// Number of hammers to perform per iteration
#define HAMMERS_PER_ITER 5000000

// TODO: RUN HISTOGRAM!
// Latency Threshold for Row Buffer Conflict
#define ROW_BUFFER_CONFLICT_LATENCY (390)

// TODO: RUN HISTOGRAM!
// Latency Threshold for Row Buffer Hit
#define ROW_BUFFER_HIT_LATENCY (290)
#define PAGE_SIZE_BITS (12)

// ROW BITS: DETERMINED BY decode-dimm.
#define ADDR_ROW_BITS (16)

// BIT XOR BITS: May need to be guesstimated.
#define ADDR_BIT_XOR_BITS (13)

#define BUCKET_LAT_STEP (20)
#define NUM_LAT_BUCKETS (50)

// Number of Rows
// Number of Columns

// Number of Banks
#define NUM_BANKS (8)

// Number of Ranks
#define NUM_RANKS (1)

// Number of channels
#define NUM_CHANNELS (1)

#endif
