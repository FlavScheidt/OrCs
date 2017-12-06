#include "stride.hpp"
/*************************************************
    STRIDE TABLE
**************************************************/
strideTable::strideTable()
{

}

void strideTable::allocate()
{
	this->nStrides = 0;
	this->stridesHits = 0;
	this->stridesTrash = 0;

    for (int i=0; i<STRIDE_LINES; i++)
    {
        this->lines[i].tag = 0;
        this->lines[i].lastAddress = 0;
        this->lines[i].stride = 0;
        this->lines[i].strideState = INVALIDO;
        this->lines[i].lruInfo = 0;
    }
}

int strideTable::search(uint64_t pc)
{

	uint64_t lru = 0;
	int oldestIndex = 0;

	for (int i=0; i<STRIDE_LINES; i++)
	{
		if (this->lines[i].tag != pc)
		{
			if (this->lines[i].strideState != INVALIDO)
			{
				if (this->lines[i].lruInfo < lru)
				{
					oldestIndex = i;
					lru = this->lines[i].lruInfo;
				}
			}
			else
				return i;
		}
		else
			return i;
	}
	return oldestIndex;
}

uint64_t strideTable::read(uint64_t pc, uint64_t address, uint64_t cycle)
{
	this->nStrides++;

	int index = this->search(pc);
	this->lines[index].lruInfo = cycle;

	if (this->lines[index].tag == pc)
	{
		if (this->lines[index].lastAddress + this->lines[index].stride == address)
		{
			this->stridesHits++;
			if (this->lines[index].strideState == TREINAMENTO)
				this->lines[index].strideState = ATIVO;

			this->lines[index].lastAddress = address;
			return this->lines[index].stride + pc;
		}
		else 
		{
			if (this->lines[index].strideState == ATIVO)
			{
				this->lines[index].strideState = INVALIDO;
				this->stridesTrash++;
			}
			else
			{
				this->lines[index].stride = address - this->lines[index].lastAddress;
				this->lines[index].lastAddress = address;
			}
			
			return 0;
		}
	}
	else
	{
		this->lines[index].tag = pc;
		this->lines[index].strideState = TREINAMENTO;
		this->lines[index].stride = 0;
		this->lines[index].lastAddress = address;

		return 0;
	}
}