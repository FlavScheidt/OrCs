#include "simulator.hpp"

/*************************************************************
    GHB
**************************************************************/
GHB::GHB()
{

}

void GHB::allocate()
{
	this->size = GHB_LINES;
	for(uint i = 0; i < this->size; i++)
	{
		this->lines[i].address = 0;
		this->lines[i].next = -1;
	}
	this->head = 0;
}

uint32_t GHB::push(uint64_t address, int32_t lastIndex)
{
	int index = this->head;

	this->lines[head].address = address;
	this->lines[head].next = lastIndex;
	this->head = (this->head + 1) % this->size;
	
	return index;
}

uint32_t GHB::access(uint64_t address, uint32_t lastIndex, uint32_t deltaBuffer[DELTA_BUFFER_SIZE], uint32_t *deltaBufSize)
{
	uint64_t addr = address;
	uint32_t ind = lastIndex;
	int32_t n_ind = ind;
	uint32_t i = 0;

	int64_t diff = address - this->lines[lastIndex].address;
	if(diff > -64 && diff < 64)
	{
	  *deltaBufSize = 0;
	  return lastIndex;
	}
	else
	{      
		deltaBuffer[i] = addr - this->lines[lastIndex].address;
		i++;
		addr = this->lines[lastIndex].address;
		ind = lastIndex;
		  
		GHB_entry * g_next = this->getNext(ind, &n_ind);
		while(g_next != NULL && i < DELTA_BUFFER_SIZE)
		{
			deltaBuffer[i] = addr - g_next->address;
			i++;
			addr = g_next->address;
			ind = n_ind;
			g_next = this->getNext(ind, &n_ind);
		}
		*deltaBufSize = i;
		return push(address, lastIndex);
	}
}

bool GHB::older(uint32_t ind1, uint32_t ind2)
{
	if(ind1 == ind2)
	{
		return false;
	}
	int oldest = head;
	int diff1 = oldest - ind1;
	if(diff1 < 0)
		diff1 += size;
	int diff2 = oldest - ind2;
	if(diff2 < 0)
		diff2 += size;
	if(diff1 > diff2)
		return true;
	else
		return false;
}

GHB_entry * GHB::getNext(int index, int * nextIndex)
{
	if (index == -1)
		return(NULL);

	int n_index = this->lines[index].next;

	*nextIndex = n_index;
	if(n_index == -1)
		return NULL;

	if(older(n_index, index))
		return (&this->lines[n_index]);
	else
		return NULL;
}
/*************************************************************
    INDEX Table
**************************************************************/
IndexTable::IndexTable()
{

}

void IndexTable::allocate()
{
	this->miss = 0;
	this->hits = 0;

	for (int i=0; i<INDEX_ASSOC; i++)
	{
		for (int j=0; j<(INDEX_LINES/INDEX_ASSOC); j++)
		{
			this->blocks[i].lines[j].tag = 0;
			this->blocks[i].lines[j].lruInfo = 0;
			this->blocks[i].lines[j].index = -1;
		}
	}
}

void IndexTable::insert(uint64_t pc, uint64_t lruInfo, uint32_t index)//, void * index, int indexType)
{
	int tableIndex = pc % (INDEX_LINES/INDEX_ASSOC);
	uint64_t lru = 0;

	for (int i=0; i<INDEX_ASSOC; i++)
		if (this->blocks[i].lines[tableIndex].lruInfo < lru)
			lru = i;


	this->blocks[lru].lines[tableIndex].tag = pc;
	this->blocks[lru].lines[tableIndex].lruInfo = lruInfo;
	this->blocks[lru].lines[tableIndex].index = index;

}

int32_t IndexTable::access(uint64_t pc, uint64_t lruInfo)
{
	int tableIndex = pc % (INDEX_LINES/INDEX_ASSOC);
	for (int i=0; i<INDEX_ASSOC; i++)
	{
		if (this->blocks[i].lines[tableIndex].tag == pc)
		{
			this->blocks[i].lines[tableIndex].lruInfo = lruInfo;
			return this->blocks[i].lines[tableIndex].index;
		}
	}
	return -1;
}

void IndexTable::updateIndex(uint64_t pc, uint32_t index)
{		
	int tableIndex = pc % (INDEX_LINES/INDEX_ASSOC);

	for (int i=0; i<INDEX_ASSOC; i++)
	{
		if (this->blocks[i].lines[tableIndex].tag == pc)
			this->blocks[i].lines[tableIndex].index = index;
	}
}


/*************************************************************
    LDB
**************************************************************/
LDB::LDB()
{

}

void LDB::allocate()
{
	for (int i=0; i<LDB_QUANT; i++)
	{
		this->blocks[i].tag = 0;
		this->blocks[i].lastAddress = 0;
		this->blocks[i].lruInfo = 0;
		this->blocks[i].stride = false;
		this->blocks[i].last_c_stride = 0;

		for (int j=0; j<LDB_DELTA_SIZE; j++)
			this->blocks[i].delta[j] = 0;
	}
}

bool LDB::access(uint64_t index, uint64_t pc, uint64_t lru)
{
	uint64_t indexLDB = index;// - GHB_LINES;

	if (this->blocks[indexLDB].tag == pc)
	{
		this->blocks[indexLDB].lruInfo = lru;
		return true;
	}

	return false;
}

uint32_t LDB::insert(uint64_t pc, uint64_t address, uint64_t lruInfo, uint32_t deltas[DELTA_BUFFER_SIZE], uint32_t deltaBufSize, bool stride)
{
	int index = 0;
	uint64_t lru = 0;

	for (int i=0; i<LDB_LINES; i++)
	{
		if (this->blocks[i].lruInfo < lru)
		{
			lru = this->blocks[i].lruInfo;
			index = i;
		}
	}

	this->blocks[index].tag = pc;
	this->blocks[index].lastAddress = address;
	this->blocks[index].lruInfo = lruInfo;
	this->blocks[index].stride = stride;
	this->blocks[index].last_c_stride = deltas[0];

	for (uint j=0; j<deltaBufSize; j++)
		this->blocks[index].delta[j] = deltas[j];

	for (int j=deltaBufSize; j<LDB_DELTA_SIZE; j++)
		this->blocks[index].delta[j] = 0;

	return index;
}

void LDB::updateStride(uint32_t index, bool stride)
{
	this->blocks[index].stride = stride;
}

void LDB::updateDeltas(uint32_t index, uint32_t delta)
{
	for (int i=LDB_DELTA_SIZE-1; i>0; i--)
		this->blocks[index].delta[i] = this->blocks[index].delta[i-1];

	this->blocks[index].delta[0] = delta;
}


/*************************************************************
    Bloom Filter
**************************************************************/
BloomFilter::BloomFilter()
{

}

void BloomFilter::allocate()
{
	for (int i=0; i<BLOOM_FILTER_BUCKETS; i++)
		bloom[i] = 0;

	shift = log2(BLOOM_FILTER_BUCKETS);
}

uint32_t BloomFilter::log2(uint32_t x)
{
	uint32_t r=0;
    while( (x >> r) != 0)
		r++;
    return r-1; // returns -1 for x==0, floor(log2(x)) otherwise
}

void BloomFilter::insert(uint64_t address)
{
	uint64_t index = BLOOM_FILTER_HASH;

	this->bloom[index] = 1;
}

void BloomFilter::reset()
{
	for (int i=0; i<BLOOM_FILTER_BUCKETS; i++)
		bloom[i] = 0;	
}

bool BloomFilter::search(uint64_t address)
{
	return this->bloom[BLOOM_FILTER_HASH];
}

/*************************************************************
    Prefetcher
**************************************************************/
Prefetcher::Prefetcher()
{

}

void Prefetcher::allocate()
{
	this->indexTable = new IndexTable;
	this->ghb = new GHB;
	this->ldb = new LDB;
	//this->bloomFilter = new BloomFilter;
	this->cycles = 0;

	this->indexTable->allocate();
	this->ghb->allocate();
	this->ldb->allocate();
	//this->bloomFilter->allocate();

	this->nPrefDeltaCorrelation = 0;
	this->nPrefSimpleDeltaCorr = 0;
	this->nPrefGlobalStride = 0;
	this->nPrefFromDelta = 0;
	this->nPrefFromDeltaCorr = 0;
	this->nPrefFromLStride = 0;
	this->nPrefFromGlobalStride = 0;

	this->cycles = 0;
	this->nPrefetches = 0;
	this->usefulPrefetches = 0;
}

uint32_t Prefetcher::access(uint64_t address, uint64_t pc, uint64_t prefetchAddress[LDB_DELTA_SIZE])
{
	uint32_t deltaBufSize = 0;
	int32_t index = 0;
	uint32_t indexLDB = 0;
	uint32_t indexGHB = pc % GHB_LINES;
	int32_t lastIndex = 0;
	uint32_t nPrefetches = 0;
	bool strongMatch = false;
	uint32_t c_stride = 0;

	//Search Index Table
	if ((index = this->indexTable->access(pc, this->cycles)) == -1)
	{
		//New entry on the GHB
		this->ghb->push(address, -1);
		//New entry on the index table
		this->indexTable->insert(pc, this->cycles, indexGHB);

		this->indexTable->miss++;
		return 0;
	}
	else // index found (index table)
	{
		this->nPrefetches++;
		if (index < GHB_LINES) //is the index on the GHB or in the LDBs?
		{
			lastIndex = index;
			index = this->ghb->access(address, lastIndex, this->deltaBuffer, &deltaBufSize);
			this->indexTable->updateIndex(pc, index);

			if((lastIndex != index) && (this->ghb->older(lastIndex, index)))
			{
				//Delta Correlation
				nPrefetches = this->deltaCorrelation(deltaBufSize, address, prefetchAddress, &strongMatch);
				nPrefDeltaCorrelation = nPrefetches;

				if (nPrefetches > 0 && nPrefetches <= PREFETCH_DEGREE) //Delta Correlation
				{
					indexLDB = this->ldb->insert(pc, address, this->cycles, this->deltaBuffer, deltaBufSize, false);

					if (nPrefetches == PREFETCH_DEGREE)
					{
						if(((prefetchAddress[0] == address + this->deltaBuffer[0]) && (prefetchAddress[3] == address + 4 * this->deltaBuffer[0]) && (prefetchAddress[7] == address + 8 * this->deltaBuffer[0])) || strongMatch)
						{
							this->ldb->updateStride(index, true);
						}
					}
					this->indexTable->updateIndex(pc, indexLDB + GHB_LINES);
				}
				else if (nPrefetches == 0) //Simple Delta Correlation
				{						
					nPrefetches = nPrefetches + this->simpleDeltaCorrelation(deltaBufSize, address, prefetchAddress);
					nPrefSimpleDeltaCorr = nPrefetches;

				}
				
				if (nPrefetches > PREFETCH_DEGREE) //Global Stride
				{
					nPrefGlobalStride = nPrefetches;
					nPrefetches = nPrefetches + this->globalStride(index, address, prefetchAddress, nPrefetches);
					nPrefGlobalStride = nPrefetches - nPrefGlobalStride;
				}
			}
			return nPrefetches;
		}
		else //Access LDBS
		{
			lastIndex = index - GHB_LINES;

			if (this->ldb->access(lastIndex, pc, this->cycles))
			{
				c_stride = address - this->ldb->blocks[lastIndex].lastAddress;
				this->ldb->updateDeltas(lastIndex, c_stride);

				nPrefetches = deltaCorrelation(deltaBufSize, address, prefetchAddress, &strongMatch);

				if (nPrefetches == PREFETCH_DEGREE)
				{
					if ((prefetchAddress[0] == address + c_stride) && (prefetchAddress[3] == address) + (4*c_stride && prefetchAddress[7] == address) + (8 * c_stride || strongMatch))
					{
						if (this->ldb->blocks[lastIndex].stride == true)
						{
							prefetchAddress[0] = prefetchAddress[PREFETCH_DEGREE - 1];
							nPrefetches = 1;
						}
						else
						{
							this->ldb->updateStride(lastIndex, true);
						}
					}
					else
					{
						this->ldb->updateStride(lastIndex, false);
					}
				}
				else
				{
					this->ldb->updateStride(lastIndex, false);
				}

				if (nPrefetches == 0)
					nPrefetches = nPrefetches + this->simpleDeltaCorrelation(deltaBufSize, address, prefetchAddress);

				nPrefDeltaCorrelation = nPrefetches;
				nPrefFromDelta = nPrefFromDelta + nPrefetches;

				if (nPrefetches == 0)
				{
					nPrefetches++;
					prefetchAddress[0] = address + this->ldb->blocks[lastIndex].last_c_stride;
					prefetchAddress[1] = address + 64;
					nPrefetches++;
				}
				else
				{
					this->ldb->updateStride(lastIndex, c_stride);
				}
				return nPrefetches;
			}
			else
			{
				this->indexTable->updateIndex(pc, this->ghb->push(address, -1));
				return 0;
			}
		}
	}
}

uint32_t Prefetcher::deltaCorrelation(uint32_t deltaBufSize, uint64_t address, uint64_t prefetchAddress[LDB_DELTA_SIZE], bool * strongMatch)
{
	uint64_t addr=0;
	int32_t patternBroken=0;
	int32_t nPref=0;

	*strongMatch = false;

	if (deltaBufSize >= 4)
	{
		for (uint i=2; i<deltaBufSize-1; i++)
		{
			if ((this->deltaBuffer[i+2] == this->deltaBuffer[0]) && (this->deltaBuffer[i+1] == this->deltaBuffer[1])) //Search for a pattern 
			{
				if ((i < deltaBufSize-2) && (this->deltaBuffer[i+2] == this->deltaBuffer[2])) //Search for a string pattern
					*strongMatch = true;

				addr = address;
				patternBroken = i;
				nPref = LDB_DELTA_SIZE;

				for (int j=0; j<nPref; j++) //Set the prefetched address
				{
					addr = addr + this->deltaBuffer[i-1];
					prefetchAddress[j] = addr;
					i--;

					if (i == 0)
						i = patternBroken;
				}
				this->nPrefFromDeltaCorr = this->nPrefFromDeltaCorr + this->nPrefetches;
				return nPref;
			}
		}
	}
	return 0;
}

uint32_t Prefetcher::simpleDeltaCorrelation(uint32_t deltaBufSize, uint64_t address, uint64_t prefetchAddress[LDB_DELTA_SIZE])
{
	int32_t prefetchDegree = 4;
	uint64_t addr=0;
	int32_t patternBroken=0;
	// int32_t nPref;

	if (deltaBufSize >= 2)
	{
		for (uint i=1; i<deltaBufSize; i++)
		{
			if (this->deltaBuffer[i] == this->deltaBuffer[0])
			{
				patternBroken = i;
				addr = address;

				for (int j=0; j<prefetchDegree; j++)
				{
					addr = addr + this->deltaBuffer[i-1];
					prefetchAddress[j] = addr;
					i--;

					if (i == 0)
						i = patternBroken;
				}
			
				this->nPrefFromLStride = this->nPrefFromLStride + this->nPrefetches;
				return prefetchDegree;
			}
		}
	}
	return 0;
}

uint32_t Prefetcher::globalStride(uint32_t index, uint64_t address, uint64_t prefetchAddress[LDB_DELTA_SIZE], uint32_t nextPrefSlot)
{
	int32_t index1=0; // pointer to the first(younger) GHB entry of PC1
	int32_t index2=0; // pointer to the second(older) GHB entry of PC1
	uint32_t stride1=0;
	uint32_t stride2=0;
	uint32_t count = 0;
	int32_t nMisses=0; // number of global misses between index1 and index2
	uint64_t addr = 0;

	index1 = this->ghb->lines[index].next;
	if(index1 == -1)
		return 0;

	index2 = this->ghb->lines[index1].next;
	if(index2 == -1)
		return 0;

	if(this->ghb->older(index2, index1) == false)
		return 0;

	nMisses = index1 - index2;
	if (nMisses < 0)
		nMisses = nMisses + GHB_LINES;
	nMisses--;

	for (int i=(nMisses-1); i>=0 && nextPrefSlot < LDB_DELTA_SIZE; i--)
	{
		stride1 = this->ghb->lines[((index1+i+1) % this->ghb->size)].address - this->ghb->lines[index1].address;
		stride2 = this->ghb->lines[((index2+i+1) % this->ghb->size)].address - this->ghb->lines[index2].address;

		if (stride1 == stride2)
		{
			addr = (address & stride1) & 0xffffffc0;

			if (addr != (address & 0xffffffc0))
			{
				prefetchAddress[nextPrefSlot] = addr;
				nPrefFromGlobalStride++;
				count++;
				nextPrefSlot++;
			}
		}
	}
	return count;
}
