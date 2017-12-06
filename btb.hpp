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

#define BTB_ASSOC 4
#define BTB_LINES 512
#define MISS_PENALTY 8

/*************************************************************
    BTB
**************************************************************/
typedef struct line {
    uint64_t tag; //pc
    uint64_t lruInfo;
    uint64_t targetAddress;
    bool valid;
    bool bht;
    bool bht_ant;
} line;

typedef struct block {
    uint32_t id;
    line lines[BTB_LINES/BTB_ASSOC];
} block;

class BTB {
    public:

    block blocks[BTB_ASSOC];
    uint32_t misses;
    uint32_t hits;

    uint32_t right_address;
    uint32_t wrong_address;

    uint32_t right1;
    uint32_t wrong1;
    uint32_t right2;
    uint32_t wrong2;
    uint64_t cycle1;
    uint64_t cycle2;

    BTB();
    bool search(uint64_t pc, uint32_t *nBlock);
    void insert(opcode_package_t op, bool bht);//line toInsert);
    void allocate();
    void exec(opcode_package_t new_instruction, opcode_package_t next_instruction, bool bht);
};
