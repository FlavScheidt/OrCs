#include "simulator.hpp"

/*************************************************
    BTB
*************************************************/
void BTB::insert(opcode_package_t op, bool bht)
{
    uint64_t index;
    uint32_t earlierLRU, earlierIndex;

    index = (op.opcode_address >> 2) & 127;
    earlierLRU=this->blocks[0].lines[index].lruInfo;
    earlierIndex = 0;

    for (int i=1; i<BTB_ASSOC; i++)
    {
        if (earlierLRU < this->blocks[i].lines[index].lruInfo)
        {
            earlierLRU = this->blocks[i].lines[index].lruInfo;
            earlierIndex = i;
        }
    }
    this->blocks[earlierIndex].lines[index].tag = op.opcode_address;
    this->blocks[earlierIndex].lines[index].lruInfo = this->cycle1;
    this->blocks[earlierIndex].lines[index].targetAddress = 0;
    this->blocks[earlierIndex].lines[index].valid = true;
    this->blocks[earlierIndex].lines[index].bht_ant = this->blocks[earlierIndex].lines[index].bht;
    this->blocks[earlierIndex].lines[index].bht = bht;
}

bool BTB::search(uint64_t pc, uint32_t *nBlock)
{
    uint64_t index = (pc >> 2) & 127;
    uint32_t i;

    for (i=0; i<BTB_ASSOC; i++)
    {
        if (this->blocks[i].lines[index].tag == pc)
        {
            *nBlock = i;
            return 1;
        }
    }
    return 0;
}

BTB::BTB()
{

}

void BTB::allocate()
{
    this->hits = 0;
    this->misses = 0;
    this->right1 = 0;
    this->wrong1 = 0;
    this->right2 = 0;
    this->wrong2 = 0;
    this->cycle1 = 0;
    this->cycle2 = 0;
    this->right_address = 0;
    this->wrong_address = 0;
    for (int i = 0; i<BTB_ASSOC; i++)
    {
        this->blocks[i].id=i;
        for (int j = 0; j<(BTB_LINES/BTB_ASSOC); j++)
        {
            this->blocks[i].lines[j].tag = 0;
            this->blocks[i].lines[j].lruInfo = 0;
            this->blocks[i].lines[j].targetAddress = 0;
            this->blocks[i].lines[j].valid = false;
            this->blocks[i].lines[j].bht = 0;
            this->blocks[i].lines[j].bht_ant = 0;            
        }
    }
}

void BTB::exec(opcode_package_t new_instruction, opcode_package_t next_instruction, bool bht)
{
    uint32_t nBlock;
    bool prediction;
    uint64_t index = (new_instruction.opcode_address >> 2) & 127;

    //Search the btb, if there is an entry, look at the bht
    if (this->search(new_instruction.opcode_address, &nBlock))
    {
        this->blocks[nBlock].lines[index].lruInfo = this->cycle1;
        this->hits++;

        if (this->blocks[nBlock].lines[index].bht_ant == false)
            prediction = false;
        else
            prediction = true;

        if(this->blocks[nBlock].lines[index].targetAddress == next_instruction.opcode_address)
            this->right_address++;
        else
        {
            this->wrong_address++;
            this->blocks[nBlock].lines[index].targetAddress = next_instruction.opcode_address;
        }

        //Is the prediction right?
        if (prediction == bht)
        {
            this->right2++;
            this->cycle1++;
            this->cycle2++;
        }
        else
        {
            this->cycle1 = this->cycle1+MISS_PENALTY;
            this->cycle2 = this->cycle2+MISS_PENALTY;
            this->wrong2++;
        }

        if (this->blocks[nBlock].lines[index].bht == bht)
            this->right1++;
        else
        {
            this->wrong1++;
            this->blocks[nBlock].lines[index].bht = bht;
        }

        this->blocks[nBlock].lines[index].bht_ant = this->blocks[nBlock].lines[index].bht;
        this->blocks[nBlock].lines[index].bht = bht;

    }
    //If theres no entry, inserts
    else
    {
        this->insert(new_instruction, bht);
        this->misses++;
        this->cycle1 = this->cycle1+MISS_PENALTY;
        this->cycle2 = this->cycle2+MISS_PENALTY;

        if (bht == true)
        {
            this->wrong1++;
            this->wrong2++;
        }
        else
        {
            this->right1++;
            this->right2++;
        }

    }
}
