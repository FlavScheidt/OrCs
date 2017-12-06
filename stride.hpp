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

#define STRIDE_LINES 16

/*************************************************************
    Stride Table
**************************************************************/
enum stride_state {
    INVALIDO=0, 
    TREINAMENTO=1, 
    ATIVO=2
};

typedef struct strideLine{
    uint64_t tag;
    uint64_t lastAddress;
    uint32_t stride;
    uint64_t lruInfo;
    stride_state strideState;
} strideLine;

class strideTable
{
    public:
        uint64_t nStrides;
        uint64_t stridesHits;
        uint64_t stridesTrash;

        strideLine lines[STRIDE_LINES];

        strideTable();
        void allocate();
        int search(uint64_t pc);
        uint64_t read(uint64_t pc, uint64_t address, uint64_t cycle);

};

