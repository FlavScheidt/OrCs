/// C Includes
#include <unistd.h>     /* for getopt */
#include <getopt.h>     /* for getopt_long; POSIX standard getopt is in unistd.h */
#include <inttypes.h>   /* for uint32_t */
#include <zlib.h>

/// C++ Includes
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>

#define CACHE_L1_ASSOC 4
#define CACHE_L2_ASSOC 8
#define CACHE_L1_SIZE 64
#define CACHE_L1_LINES 1024
#define CACHE_L2_LINES 16000
#define CACHE_L2_SIZE 1000
#define CACHE_L1_MISS_PENALTY 1
#define CACHE_L2_MISS_PENALTY 4
#define RAM_MISS_PENALTY 200
#define RAM_SIZE 4000000

/*************************************************************
    CACHE L2
**************************************************************/
typedef struct cacheLine {
    uint64_t tag;
    bool val;
    bool dirty;
    uint64_t lruInfo;
    uint64_t readyCycle;
    bool stride;
    bool prefetches;
} cacheLine;

typedef struct cacheBlockL2 {
    uint32_t id;
    cacheLine lines[CACHE_L2_LINES/CACHE_L2_ASSOC];
} cacheBlockL2;

class cacheL2
{
    public:
        cacheBlockL2 blocks[CACHE_L2_ASSOC];
        uint32_t misses;
        uint32_t hits;
        uint64_t cycles;

        cacheL2();
        void allocate();
        void write(uint64_t address, uint64_t cycles, int type);
        void read(uint64_t address, strideTable *stride, uint64_t pc, Prefetcher * prefetcherL2);
        int search(uint64_t address, uint64_t);
        void makeDirty(uint64_t tag);
};


// /*************************************************************
//     CACHE L1
// **************************************************************/
typedef struct cacheBlock {
    uint32_t id;
    cacheLine lines[CACHE_L1_LINES/CACHE_L1_ASSOC];
} cacheBlock;

class cacheL1
{
    public:
        cacheBlock blocks[CACHE_L1_ASSOC];
        uint32_t misses;
        uint32_t hits;
        uint64_t cycles;

        cacheL1();
        void allocate();
        int search(uint64_t address, uint64_t cycle);
        void read(uint64_t address, cacheL2 *L2, strideTable *stride, uint64_t pc, Prefetcher * prefetcherL1, Prefetcher * prefetcherL2);
        void write(uint64_t address, cacheL2 *L2, uint64_t cycle, int type);
};

