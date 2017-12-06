#include "branch_predictor.hpp"
/**********************************************************************
    BRANCH PREDICTOR
**********************************************************************/

bPredictor::bPredictor()
{

}

bPredictorPerceptron::bPredictorPerceptron()
{

}

void bPredictor::allocate()
{
//    this->yout_threshold = (1.93*MAX_H)+14; //Given by the paper
    this->yout_threshold = (int) (2.14 * (MAX_H+1) + 20.58);

    this->max_weight = (1<<(8-1))-1;
    this->min_weight = -(max_weight+1);

    for (uint32_t n=0; n<(MAX_N); n++)
        for (uint32_t h=0; h<(MAX_H); h++)
            this->W[n][h] = 0;

    for (uint32_t h=0; h<MAX_H; h++)
    {
        this->R[h] = 0;
        this->SR[h] = 0;
        this->G[h] = 0;
        this->SG[h] = 0;
        this->H[h] = 0;
        this->v[h] = 0;
        this->Sv[h] = 0;
    }
}

void bPredictorPerceptron::allocate()
{
    this->G = 0;
    this->yout_threshold = (int) (2.14 * (MAX_H+1) + 20.58); //Given by the paper

    for (uint32_t n=0; n<(MAX_N); n++)
        for (uint32_t h=0; h<(MAX_H); h++)
            this->W[n][h] = 0;
}

void bPredictorPerceptron::train(uint64_t i, int8_t yout, bool prediction, bool outcome)
{
    uint16_t g;

    if ((prediction != outcome) || (yout <= this->yout_threshold))
    {
        if (outcome == 1)
            this->W[i][0] = this->W[i][0]+1;
        else
            this->W[i][0] = this->W[i][0]-1;

        for (uint h=1; h<MAX_H; h++)
        {
            g = (G << (16 - h)) >> 15;

            if (outcome == g)
                this->W[i][h] = this->W[i][h]+1;
            else
                this->W[i][h] = this->W[i][h]-1;
        }
    }

    G = (G << 1) | outcome;
}

bool bPredictorPerceptron::prediction(uint64_t pc, bool outcome)
{
    uint64_t i = pc % MAX_N;
    //std::cout << i << '\n';
    int8_t yout = this->W[i][0];
    uint16_t g;

    for (uint h=1; h < MAX_H; h++)
    {
        g = (G << (16 - h)) >> 15;
        //std::cout << "G " << g << '\n';
        if (g == 1)
            yout = yout + W[i][h];
        else
            yout = yout - W[i][h];
    }

    //
    if (yout >= 0)
    {
        this->train(i, yout, true, outcome);
        SG = (G << 1) | true;
        return true;
    }
    this->train(i, yout, false, outcome);
    SG = (G << 1) | false;
    return false;
}


void bPredictor::shift_int (uint32_t v[MAX_H], uint32_t x) 
{
    for (int h=1; h<MAX_H; h++)
        v[h] = v[h-1];
    v[0] = x;
}

    // shift a Boolean into an array; for histories

void bPredictor::shift_bool (bool v[MAX_H], bool x) 
{
    for (int h=2; h<MAX_H; h++)
        v[h] = v[h-1];
    v[1] = x;
}


int bPredictor::satincdec (int w, bool inc) 
{
    if (inc)
    {
        if (w < max_weight)
            w++;
    }
    else
    {
        if (w > min_weight) 
            w--;
    }
    return w;
}

void bPredictor::train(uint64_t i, int8_t yout, bool prediction, bool outcome)
{
    uint32_t k;

   for (uint h=1; h < MAX_H; h++)
   {
        k = MAX_H - h;
        if (outcome == 1)
            this->R[k+1] = this->R[k] + this->W[i][h];
        else
            this->R[k+1] = this->R[k] - this->W[i][h];
    }
    this->R[0] = 0;

    shift_bool(G, outcome);
    shift_int(v, i);

    if (outcome != prediction)
        for (int h=0; h<MAX_H;h++)
        {
            SR[h] = R[h];
            SG[h] = G[h];
            Sv[h] = v[h];
        }

    if ((prediction != outcome) || (yout <= this->yout_threshold))
    {
        if (outcome == 1)
            this->W[i][0] = this->W[i][0]+1;
        else
            this->W[i][0] = this->W[i][0]-1;

        for (uint h=1; h<MAX_H; h++)
        {
            k = this->v[h];

            if (outcome == H[h])
                this->W[k][h] = satincdec(W[k][h], true);
            else
                this->W[k][h] = satincdec(W[k][h], false);

        }
    }

}

bool bPredictor::prediction(uint64_t pc, bool outcome)
{
    uint64_t i;
    int8_t yout;
    uint32_t k;
    bool prediction;

    i = (pc % MAX_N);

    shift_int(Sv, i);

    for (int h=0; h<MAX_H; h++)
    {
        v[h] = Sv[h];
        H[h] = SG[h];
    }

    yout = (this->SR[MAX_H-1] + this->W[i][0]);

    if (yout >= 0)
        prediction = true;
    else 
        prediction = false;

    //Sets speculative R
    for (uint32_t h=1; h<MAX_H; h++)
    {   
        k = MAX_H - h;
        if (prediction == true)
            this->SR[k+1] = this->SR[k] + this->W[i][h];
        else
            this->SR[k+1] = this->SR[k] - this->W[i][h];

    }
    this->SR[0] = 0;
    shift_bool(SG, prediction);

    this->train(i, yout, prediction, outcome);

    return prediction;
}
