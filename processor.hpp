// ============================================================================
// ============================================================================
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
/********************************************************************
    PROCESSOR
********************************************************************/
class processor_t {
    public:
        BTB *btb;
        bPredictor *predictor;
        bPredictorPerceptron *perceptron;
        cacheL1 *L1;
        cacheL2 *L2;
        strideTable *stride;
        Prefetcher *prefetcherL1;
        Prefetcher *prefetcherL2;

        uint64_t cycle;

        // ====================================================================
        /// Methods
        // ====================================================================
        processor_t();
        void allocate();
        void clock();
        void statistics();
        void cache(opcode_package_t new_instruction);
};
