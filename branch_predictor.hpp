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

//#define MAX_W 256000
#define MAX_H 37
#define MAX_N 2048

/*************************************************************
    BRANCH PREDICTOR
*************************************************************/
class bPredictor {
    bool G[MAX_H]; //Shift register global history
    bool SG[MAX_H]; //Speculative shift register global history
    bool H[MAX_H];

    int8_t W[MAX_N][MAX_H]; //Weights table

    uint32_t v[MAX_H]; //Recently used indices
    uint32_t Sv[MAX_H];
    uint32_t R[MAX_H]; //Partial sums of the perceptron
    uint32_t SR[MAX_H]; //Speculative partial sums

    int32_t yout_threshold;
    int8_t max_weight;
    int8_t min_weight;

    void shift_int (uint32_t v[MAX_H], uint32_t x);
    void shift_bool (bool v[MAX_H], bool x);
    int satincdec (int w, bool inc);

    public:
        uint32_t right;
        uint32_t wrong;
        uint64_t cycle;

        bPredictor();
        void allocate();
        bool prediction (uint64_t pc, bool outcome);
        void train (uint64_t i, int8_t yout, bool prediction, bool outcome);

};

class bPredictorPerceptron {
    uint16_t G; //Shift register global history
    uint16_t SG;

    int8_t W[MAX_N][MAX_H]; //Weights table

    uint32_t R[MAX_H]; //Partial sums of the perceptron

    int32_t yout_threshold;

    public:
        uint32_t right;
        uint32_t wrong;
        uint64_t cycle;


        bPredictorPerceptron();
        void allocate();
        bool prediction (uint64_t pc, bool outcome);
        void train (uint64_t i, int8_t yout, bool prediction, bool outcome);

};
