#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>

#define INDEX_ASSOC 8
#define INDEX_LINES 256

#define GHB_LINES 128

#define LDB_LINES 16
#define LDB_QUANT 16
#define LDB_DELTA_SIZE 8

#define BLOOM_FILTER_BUCKETS 2048
#define BLOOM_FILTER_HASH (((address & 0xffffffff) >> shift) ^ (address & 0xffffffff)) % BLOOM_FILTER_BUCKETS

#define DELTA_BUFFER_SIZE 32

#define PREFETCH_DEGREE 7

/*************************************************************
    GHB
**************************************************************/
typedef struct GHB_entry
{
	uint64_t address;
	int32_t next;
} GHB_entry;

class GHB
{
	public:
		GHB_entry lines[GHB_LINES];
		uint32_t head;
		uint32_t size;

		GHB();
		void allocate();

		uint32_t push(uint64_t address, int32_t lastIndex);
		uint32_t access(uint64_t address, uint32_t lastIndex, uint32_t deltaBuffer[DELTA_BUFFER_SIZE], uint32_t *deltaBufSize);
		bool older(uint32_t i1, uint32_t i2);
		GHB_entry * getNext(int index, int * nextIndex);
};

/*************************************************************
    INDEX Table
**************************************************************/
typedef struct index_line
{
	uint64_t tag;
	uint64_t lruInfo;
	int index;
} index_line;

typedef struct index_block
{
	index_line lines[INDEX_LINES/INDEX_ASSOC];
} index_block;

class IndexTable
{
	public:
		index_block blocks[INDEX_ASSOC];

		uint32_t miss;
		uint32_t hits;


	IndexTable();
	void allocate();

	void insert(uint64_t pc, uint64_t lruInfo, uint32_t index);
	int32_t access(uint64_t pc, uint64_t lruInfo);
	void updateIndex(uint64_t pc, uint32_t index);
	// void search();
};

/*************************************************************
    LDB
**************************************************************/
typedef struct LDB_entry
{
	uint64_t tag;
	uint64_t lastAddress;
	uint64_t delta[LDB_DELTA_SIZE]; //FIFO
	uint64_t lruInfo;
	bool stride;
	uint32_t last_c_stride; //last correct stride
} LDB_entry;

class LDB
{
	public:
		LDB_entry blocks[LDB_QUANT];

		LDB();
		void allocate();

		bool access(uint64_t index, uint64_t pc, uint64_t lru);
 		uint32_t insert(uint64_t pc, uint64_t address, uint64_t lruInfo, uint32_t deltas[LDB_DELTA_SIZE], uint32_t deltaBufSize, bool stride);
 		void updateStride(uint32_t index, bool stride);
 		void updateDeltas(uint32_t index, uint32_t delta);
		// void insert();
		// void remove();
		// void search();
};

/*************************************************************
    Bloom Filter
**************************************************************/
class BloomFilter
{
	public:
		bool bloom[BLOOM_FILTER_BUCKETS];

		BloomFilter();
		void allocate();

		void insert(uint64_t address);
		void reset();
		bool search(uint64_t);

	private:
		uint64_t shift;

		uint32_t log2(uint32_t x);
};

/*************************************************************
    Prefetcher
**************************************************************/
class Prefetcher
{
	public:
		IndexTable *indexTable;
		GHB *ghb;
		LDB *ldb;
		//BloomFilter *bloomFilter;

		uint32_t deltaBuffer[DELTA_BUFFER_SIZE];

		uint64_t cycles;
		uint32_t nPrefetches;
		uint32_t usefulPrefetches;

		uint32_t nPrefDeltaCorrelation;
		uint32_t nPrefSimpleDeltaCorr;
		uint32_t nPrefGlobalStride;
		uint32_t nPrefFromDelta;
		uint32_t nPrefFromDeltaCorr;
		uint32_t nPrefFromLStride;
		uint32_t nPrefFromGlobalStride;

		Prefetcher();
		void allocate();

		uint32_t access(uint64_t address, uint64_t pc, uint64_t prefetchAddress[LDB_DELTA_SIZE]);

		uint32_t deltaCorrelation(uint32_t deltaBufSize, uint64_t address, uint64_t prefetchAddress[LDB_DELTA_SIZE], bool * strongMatch);
		uint32_t simpleDeltaCorrelation(uint32_t deltaBufSize, uint64_t address, uint64_t prefetchAddress[LDB_DELTA_SIZE]);
		uint32_t globalStride(uint32_t index, uint64_t address, uint64_t prefetchAddress[LDB_DELTA_SIZE], uint32_t nextPrefSlot);

		// void mostCommonStride();
};