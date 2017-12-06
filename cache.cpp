#include "simulator.hpp"
/*************************************************
    CACHE L1
**************************************************/
cacheL1::cacheL1()
{

}

void cacheL1::allocate()
{
    for (int i=0; i<(CACHE_L1_ASSOC); i++)
        for (int j=0; j<(CACHE_L1_LINES/CACHE_L1_ASSOC); j++)
        {
            this->blocks[i].lines[j].tag = 0;
            this->blocks[i].lines[j].val = false;
            this->blocks[i].lines[j].dirty = false;
            this->blocks[i].lines[j].lruInfo = 0;
            this->blocks[i].lines[j].readyCycle = 0;
            this->blocks[i].lines[j].stride = false;
            this->blocks[i].lines[j].prefetches = false;
        }

    this->misses=0;
    this->hits=0;
    this->cycles=0;
}

int cacheL1::search(uint64_t address, uint64_t cycle)
{
    uint64_t tag = (address >> 6);
    uint64_t index = (tag & 1023) % (CACHE_L1_LINES/CACHE_L1_ASSOC);

    for (int i=0; i<CACHE_L1_ASSOC; i++)
        if (this->blocks[i].lines[index].tag == tag)
            if (this->blocks[i].lines[index].readyCycle >= cycle)
                return 1;
    return 0;
}

void cacheL1::read(uint64_t address, cacheL2 *l2, strideTable *stride, uint64_t pc, Prefetcher * prefetcherL1, Prefetcher * prefetcherL2)
{
    uint64_t tag = (address >> 6);
    uint64_t index = (tag & 1023) % (CACHE_L1_LINES/CACHE_L1_ASSOC);

    uint64_t prefetchedAddress[LDB_DELTA_SIZE];
    uint32_t nPrefetchedAddress;

    for (int i=0; i<LDB_DELTA_SIZE; i++)
        prefetchedAddress[i] = 0;

    this->cycles = this->cycles + CACHE_L1_MISS_PENALTY;

    for (int i=0; i<CACHE_L1_ASSOC; i++)
    {  
        if (tag == this->blocks[i].lines[index].tag 
            && this->blocks[i].lines[index].val == true 
            && this->blocks[i].lines[index].dirty == false)
        {
            this->blocks[i].lines[index].lruInfo = this->cycles;
            this->hits++;

            if (this->blocks[i].lines[index].prefetches)
            {
                prefetcherL1->usefulPrefetches++;
            }
            return;
        }
    }

    this->misses++;
    // nPrefetchedAddress = prefetcherL1->access(address, pc, prefetchedAddress);

    // for (uint i=0; i<nPrefetchedAddress; i++)
    //     if (!this->search(prefetchedAddress[i], this->cycles))
    //         this->write(prefetchedAddress[i], l2, this->cycles, 2);

    l2->read(address, stride, pc, prefetcherL2);

    this->write(address, l2, 0, 0);
}

void cacheL1::write(uint64_t address, cacheL2 *l2, uint64_t cycle, int type)
{
    uint64_t earlierLRU;
    uint32_t earlierI;

    this->cycles = this->cycles + CACHE_L1_MISS_PENALTY;

    uint64_t tag = (address >> 6);
    uint64_t index = (tag & 1023) % (CACHE_L1_LINES/CACHE_L1_ASSOC) ;

    earlierLRU = 0;
    earlierI = 0;

    for (int i=0; i<CACHE_L1_ASSOC; i++)
    {
        if (this->blocks[i].lines[index].dirty == true)
        {
            this->blocks[i].lines[index].tag = tag;
            this->blocks[i].lines[index].dirty = false;
            this->blocks[i].lines[index].lruInfo = this->cycles;
            this->blocks[i].lines[index].val = true;
            this->blocks[i].lines[index].readyCycle = cycle + RAM_MISS_PENALTY;
            if (type == 1)
                this->blocks[i].lines[index].stride = true;
            else if (type == 2)
                this->blocks[i].lines[index].prefetches = true;
            else
            {
                this->blocks[i].lines[index].stride = false;
                this->blocks[i].lines[index].prefetches = false;
            }

            //l2->makeDirty(this->blocks[i].lines[index].tag);

            return;
        }

        if (this->blocks[i].lines[index].lruInfo < earlierLRU)
        {
            earlierLRU = this->blocks[i].lines[index].lruInfo;
            earlierI = i;
        }
    }

    //Flag dirty=1 on l2 and moves it to ram
   // l2->makeDirty(this->blocks[earlierI].lines[index].tag);

    this->blocks[earlierI].lines[index].tag = tag;
    this->blocks[earlierI].lines[index].val = true;
    this->blocks[earlierI].lines[index].dirty = false;
    this->blocks[earlierI].lines[index].lruInfo = this->cycles;
    this->blocks[earlierI].lines[index].readyCycle = cycle + RAM_MISS_PENALTY;    
    if (type == 1)
        this->blocks[earlierI].lines[index].stride = true;
    else if (type == 2)
        this->blocks[earlierI].lines[index].prefetches = true;
    else
    {
        this->blocks[earlierI].lines[index].stride = false;
        this->blocks[earlierI].lines[index].prefetches = false;
    }


}

/*************************************************
    CACHE L2
**************************************************/
cacheL2::cacheL2()
{

}

void cacheL2::allocate()
{
    for (int i=0; i<(CACHE_L2_ASSOC); i++)
        for (int j=0; j<(CACHE_L2_LINES/CACHE_L2_ASSOC); j++)
        {
            this->blocks[i].lines[j].tag = 0;
            this->blocks[i].lines[j].val = false;
            this->blocks[i].lines[j].dirty = false;
            this->blocks[i].lines[j].lruInfo = 0;
            this->blocks[i].lines[j].readyCycle = 0;
        }

    this->misses=0;
    this->hits=0;
    this->cycles=0;
}

void cacheL2::read(uint64_t address, strideTable *stride, uint64_t pc, Prefetcher * prefetcherL2)
{
    uint64_t strideAddress;
    uint64_t prefetchedAddress[LDB_DELTA_SIZE];
    uint32_t nPrefetchedAddress;

    for (int i=0; i<LDB_DELTA_SIZE; i++)
        prefetchedAddress[i] = 0;

    uint64_t tag = (address >> 6);
    uint64_t index = (tag & 1023) % (CACHE_L2_LINES/CACHE_L2_ASSOC);

    this->cycles = this->cycles + CACHE_L2_MISS_PENALTY;

    for (int i=0; i<CACHE_L2_ASSOC; i++)
    {  
        if (tag == this->blocks[i].lines[index].tag 
            && this->blocks[i].lines[index].val == true 
            && this->blocks[i].lines[index].dirty == false)
        {
        
            this->blocks[i].lines[index].lruInfo = this->cycles;
            this->hits++;

            if (this->blocks[i].lines[index].prefetches)
                prefetcherL2->usefulPrefetches++;

            return;
        }
    }

    this->misses++;
    this->cycles = this->cycles + RAM_MISS_PENALTY;

    // strideAddress = stride->read(pc, address, this->cycles);

    nPrefetchedAddress = prefetcherL2->access(address, pc, prefetchedAddress);
    for (uint i=0; i<nPrefetchedAddress; i++)
        if (!this->search(prefetchedAddress[i], this->cycles))
            this->write(prefetchedAddress[i], this->cycles, 2);

    this->write(address,0,0);
    // if (strideAddress != 0)
    //     if (!this->search(strideAddress, this->cycles))
    //         this->write(strideAddress, this->cycles, 1);
}

int cacheL2::search(uint64_t address, uint64_t cycle)
{
    uint64_t tag = (address >> 6);
    uint64_t index = (tag & 1023) % (CACHE_L2_LINES/CACHE_L2_ASSOC);

    for (int i=0; i<CACHE_L2_ASSOC; i++)
        if (this->blocks[i].lines[index].tag == tag)
            if (this->blocks[i].lines[index].readyCycle >= cycle)
                return 1;
    return 0;
}

void cacheL2::write(uint64_t address, uint64_t cycle, int type)
{
    uint64_t earlierLRU;
    uint32_t earlierI;

    this->cycles = this->cycles + CACHE_L2_MISS_PENALTY;

    uint64_t tag = (address >> 6);
    uint64_t index = (tag & 1023) % (CACHE_L2_LINES/CACHE_L2_ASSOC);

    earlierLRU = 0;
    earlierI = 0;
    for (int i=0; i<CACHE_L2_ASSOC; i++)
    {
        if (this->blocks[i].lines[index].dirty == true)
        {
            this->blocks[i].lines[index].tag = tag;
            this->blocks[i].lines[index].val = true;
            this->blocks[i].lines[index].dirty = false;
            this->blocks[i].lines[index].lruInfo = this->cycles;
            this->blocks[i].lines[index].readyCycle = cycle + RAM_MISS_PENALTY;
            if (type == 1)
                this->blocks[i].lines[index].stride = true;
            else if (type == 2)
                this->blocks[i].lines[index].prefetches = true;
            else
            {
                this->blocks[i].lines[index].stride = false;
                this->blocks[i].lines[index].prefetches = false;
            }

            return;
        }

        if (this->blocks[i].lines[index].lruInfo < earlierLRU)
        {
            earlierLRU = this->blocks[i].lines[index].lruInfo;
            earlierI = i;
        }
    }

    this->blocks[earlierI].lines[index].tag = tag;
    this->blocks[earlierI].lines[index].val = true;
    this->blocks[earlierI].lines[index].dirty = false;
    this->blocks[earlierI].lines[index].lruInfo = this->cycles;
    this->blocks[earlierI].lines[index].readyCycle = cycle + RAM_MISS_PENALTY;
    if (type == 1)
        this->blocks[earlierI].lines[index].stride = true;
    else if (type == 2)
        this->blocks[earlierI].lines[index].prefetches = true;
    else
    {
        this->blocks[earlierI].lines[index].stride = false;
        this->blocks[earlierI].lines[index].prefetches = false;
    }

}

void cacheL2::makeDirty(uint64_t tag)
{
    uint64_t index = (tag & 1023) % (CACHE_L2_LINES/CACHE_L2_ASSOC);

    for (int i=0; i<CACHE_L2_ASSOC; i++)
    {  
        if (tag == this->blocks[i].lines[index].tag)
        {
            this->blocks[i].lines[index].dirty = true;
            return;
        }
    }
    return;
}
