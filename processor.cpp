#include "simulator.hpp"

/*******************************************************************
    PROCESSOR
********************************************************************/

// =====================================================================
processor_t::processor_t() {


}

// =====================================================================
void processor_t::allocate() 
{
    this->btb = new BTB;
    this->predictor = new bPredictor;
    this->perceptron = new bPredictorPerceptron;

    this->L1 = new cacheL1;
    this->L2 = new cacheL2;
    
    this->L1->allocate();
    this->L2->allocate();

    this->stride = new strideTable;
    this->prefetcherL1 = new Prefetcher;
    this->prefetcherL2 = new Prefetcher;

    this->stride->allocate();
    this->prefetcherL1->allocate();
    this->prefetcherL2->allocate();
}

void processor_t::cache(opcode_package_t new_instruction)
{
    if (new_instruction.is_read)
        this->L1->read(new_instruction.read_address, this->L2, this->stride, new_instruction.opcode_address, this->prefetcherL1, this->prefetcherL2);

    if(new_instruction.is_read2)
        this->L1->read(new_instruction.read2_address, this->L2, this->stride, new_instruction.opcode_address, this->prefetcherL1, this->prefetcherL2);

    if (new_instruction.is_write)
        this->L1->write(new_instruction.write_address, this->L2, 0, 0);
}

// =====================================================================
void processor_t::clock() 
{
    // bool bht;
    // bool prediction;

    /// Get the next instruction from the trace
    opcode_package_t new_instruction;
    if (!orcs_engine.trace_reader->trace_fetch(&new_instruction)) {
        /// If EOF
        orcs_engine.simulator_alive = false;
    }
    else
    {
        // BRANCH PREDICTOR
        //is that a branch?
        // if ((INSTRUCTION_OPERATION_BRANCH == new_instruction.opcode_operation))
        // {
        //     this->btb->cycle1++;
        //     this->btb->cycle2++;
        //     this->perceptron->cycle++;
        //     this->predictor->cycle++;
        //     //std::cout << new_instruction.opcode_assembly << '\n';
        //     //taken or not taken?
        //     //takes the next instruction (which CANT be another branch) and calculates de address
        //     opcode_package_t next_instruction;
        //     if (!orcs_engine.trace_reader->trace_fetch(&next_instruction)) {
        //         /// If EOF
        //         orcs_engine.simulator_alive = false;
        //     }
        //     else
        //     {
                // //BHT
                // if (next_instruction.opcode_address == (new_instruction.opcode_address+new_instruction.opcode_size))
                //     bht = false;
                // else
                //     bht = true;

                // //BTB 1 bit
                // this->btb->exec(new_instruction, next_instruction, bht);

                // //Perceptron fast path
                // prediction=this->predictor->prediction(new_instruction.opcode_address, bht);
                // if (prediction == true)
                // {
                //     this->predictor->right++;
                //     this->predictor->cycle++;
                // }
                // else
                // {
                //     this->predictor->wrong++;
                //     this->predictor->cycle = this->predictor->cycle+MISS_PENALTY;
                // }

                // //Perceptron
                // prediction=this->perceptron->prediction(new_instruction.opcode_address, bht);
                // if (prediction == true)
                // {
                //     this->perceptron->right++;
                //     this->perceptron->cycle++;
                // }
                // else
                // {
                //     this->perceptron->wrong++;
                //     this->perceptron->cycle = this->perceptron->cycle+MISS_PENALTY;
                // }

                this->prefetcherL1->cycles++;
                this->prefetcherL2->cycles++;

                this->cache(new_instruction);

        // }
        // else
        // {
        //     // this->btb->cycle1++;
        //     // this->btb->cycle2++;
        //     // this->perceptron->cycle++;
        //     // this->predictor->cycle++;
        //     this->prefetcherL1->cycles++;
        //     this->prefetcherL2->cycles++;
        //     this->cache(new_instruction);
        // }
    }
};

// =====================================================================
void processor_t::statistics() {
    ORCS_PRINTF("######################################################\n");
    ORCS_PRINTF("processor_t\n");

    // std::cout << "#############\n";
    // std::cout << "BTB \n";
    // std::cout << "#############\n";
    // std::cout << "BTB misses: " << this->btb->misses << '\n';
    // std::cout << "BTB hits: " << this->btb->hits << '\n';
    // std::cout << "Right Address predictions: " << this->btb->right_address << '\n';
    // std::cout << "Worng Address predictions: " << this->btb->wrong_address << '\n';
    // std::cout << "Total: " << this->btb->right_address + this->btb->wrong_address << '\n';
    // std::cout << (float(this->btb->right_address)/float(this->btb->wrong_address+this->btb->right_address))*100 << '%' << '\n';

    // std::cout << "#############\n";
    // std::cout << "1-BIT predictor\n";
    // std::cout << "#############\n";
    // std::cout << "Cycles: " << this->btb->cycle1 << '\n';
    // std::cout << "Right predictions: " << this->btb->right1 << '\n';
    // std::cout << "Wrong predictions: " << this->btb->wrong1 << '\n';
    // std::cout << "Total: " << this->btb->right1 + this->btb->wrong1 << '\n';
    // std::cout << (float(this->btb->right1)/float(this->btb->wrong1+this->btb->right1))*100 << '%' << '\n';

    // std::cout << "#############\n";
    // std::cout << "2-BIT predictor\n";
    // std::cout << "#############\n";
    // std::cout << "Cycles: " << this->btb->cycle2 << '\n';
    // std::cout << "Right predictions: " << this->btb->right2 << '\n';
    // std::cout << "Wrong predictions: " << this->btb->wrong2 << '\n';
    // std::cout << "Total: " << this->btb->right2 + this->btb->wrong2 << '\n';
    // std::cout << (float(this->btb->right2)/float(this->btb->wrong2+this->btb->right2))*100 << '%' << '\n';

    // std::cout << "#############\n";
    // std::cout << "Perceptron\n" ;
    // std::cout << "#############\n";
    // std::cout << "Cycles: " << this->perceptron->cycle << '\n';
    // std::cout << "Right predictions: " << this->perceptron->right << '\n';
    // std::cout << "Wrong predictions: " << this->perceptron->wrong << '\n';
    // std::cout << "Total: " << this->perceptron->right + this->perceptron->wrong << '\n';
    // std::cout << (float(this->perceptron->right)/float(this->perceptron->wrong+this->perceptron->right))*100 << '%' << '\n';

    // std::cout << "#############\n";
    // std::cout << "Perceptron Fast Path\n" ;
    // std::cout << "#############\n";
    // std::cout << "Cycles: " << this->predictor->cycle << '\n';
    // std::cout << "Right predictions: " << this->predictor->right << '\n';
    // std::cout << "Wrong predictions: " << this->predictor->wrong << '\n';
    // std::cout << "Total: " << this->predictor->right + this->predictor->wrong << '\n';
    // std::cout << (float(this->predictor->right)/float(this->predictor->wrong+this->predictor->right))*100 << '%' << '\n';

    std::cout << "################\n";
    std::cout << "Cache\n";
    std::cout << "################\n";
    std::cout << "Cycles: " << this->L1->cycles+this->L2->cycles + this->predictor->cycle << "\n";

    std::cout << "################\n";
    std::cout << "Cache L1\n";
    std::cout << "################\n";
    std::cout << "Miss: " << this->L1->misses << "\n";
    std::cout << "Hits: " << this->L1->hits << "\n";
    std::cout << (float(this->L1->hits)/float(this->L1->misses+this->L1->hits))*100 << '%' << '\n';

    std::cout << "################\n";
    std::cout << "Cache L2\n";
    std::cout << "################\n";
    std::cout << "Miss: " << this->L2->misses << "\n";
    std::cout << "Hits: " << this->L2->hits << "\n";
    std::cout << (float(this->L2->hits)/float(this->L2->misses+this->L2->hits))*100 << '%' << '\n';

    std::cout << "################\n";
    std::cout << "Stride\n";
    std::cout << "################\n";
    std::cout << "Strides: " << this->stride->nStrides << "\n";
    std::cout << "Useful strides: " << this->stride->stridesHits << "\n";
    std::cout << (float(this->stride->stridesHits)/float(this->stride->nStrides))*100 << '%' << '\n';

    std::cout << "################\n";
    std::cout << "Prefetcher L2\n";
    std::cout << "################\n";
    std::cout << "Cycles: " << this->prefetcherL2->cycles << '\n';
    std::cout << "Prefectches: " << this->prefetcherL1->nPrefetches << '\n';
    std::cout << "Useful prefecthes: " << this->prefetcherL2->usefulPrefetches << '\n';
    std::cout << (float(this->prefetcherL2->usefulPrefetches)/float(this->prefetcherL2->nPrefetches))*100 << '%' << '\n';

};
