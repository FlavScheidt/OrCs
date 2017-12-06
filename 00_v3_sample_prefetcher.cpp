// Author: Martin Dimitrov
// Paper Title: Data Prefetching Using Local Delta Buffers
// email: dimitrov@cs.ucf.edu
// Date: Dec. 12 2008
// School of Electrical Engineering and Computer Science
// University of Central Florida


#include "interface.h"          // Do NOT edit interface .h

#include "sample_prefetcher.h"
#include <assert.h>

SamplePrefetcher *pref1;        // This is the L1 prefetcher
MSHR *mshr1;                    // This is the L1 MSHR
BloomFilter *filter1;
UINT32 num_filter_access1 = 0;

// 
// Function to initialize the prefetchers.  DO NOT change the prototype of this
// function.  You can change the body of the function by calling your
// necessary initialization functions.
// 

void InitPrefetchers()          // DO NOT CHANGE THE PROTOTYPE
{
    // INSERT YOUR CHANGES IN HERE
    pref1 = new SamplePrefetcher();
    mshr1 = new MSHR(1,16);
	filter1 = new BloomFilter(4096);
}



void IssueL1Prefetches(const COUNTER cycle, const PrefetchData_t * L1Data)
{

    INT32 k;
    UINT32 mshr_hit;
    UINT32 queue_full;
	UINT32 filter_hit;

for(INT32 req = 0; req < 4; req++){

    ADDRINT pref_index = L1Data[req].LastRequestAddr;
    UINT32 pref1_hit = GetPrefetchBit(0, L1Data[req].DataAddr & 0xffffffc0);
	
    if ((cycle == L1Data[req].LastRequestCycle) && ((L1Data[req].hit == 0) || (1 == pref1_hit)))
    {

		// Check the MSHR. Do not make prefetches if there is an outstanding 
		// miss in the MSHR
		if(L1Data[req].hit != 0){
			mshr_hit = mshr1->Hit(L1Data[req].DataAddr & 0xffffffc0);
		}
		else{
			mshr_hit = mshr1->AccessEntry(L1Data[req].DataAddr & 0xffffffc0);
		}

        if (0 == mshr_hit)
        {
            ADDRINT pref_addr[PREF_BUF_SIZE];
            INT32 pref_degree = 0;

			// Index into the prefetcher using PC
			// fill a buffer with prefetch addresses, pref_addr
			// return the number of prefetch addresses, pref_degree
            pref_degree = pref1->AccessPrefetcher(L1Data[req].DataAddr, pref_index, pref_addr);

            for (k = 0; k < pref_degree; k++)
            {

				// Probe the Bloom filter to eliminate redundant prefetches
				filter_hit = filter1->Hit(pref_addr[k] & 0xffffffc0);

                if (0 == filter_hit)
                {
                    queue_full = IssueL1Prefetch(cycle, pref_addr[k]);
                    if (0 == queue_full)
                    {           
						// if prefetch is not dropped, update the Bloom filter
						filter1->Update(pref_addr[k] & 0xffffffc0);
						num_filter_access1++;
						if(num_filter_access1 > 512){
							num_filter_access1=0;				
							filter1->flush();
						}
                    }
                }// filter hit
            }
        } // mshr miss
    }
}//for each request

} // IssueL1Prefetches


// 
// Function that is called every cycle to issue prefetches should the
// prefetcher want to.  The arguments to the function are the current cycle,
// the demand requests to L1 and L2 cache.  Again, DO NOT change the prototype 
// of this function.  You can change the body of the function by calling your
// necessary routines to invoke your prefetcher.
// 

// DO NOT CHANGE THE PROTOTYPE
void IssuePrefetches(COUNTER cycle, PrefetchData_t * L1Data, PrefetchData_t * L2Data)
{
    // INSERT YOUR CHANGES IN HERE

    // Release MSHR entries
    mshr1->Release(cycle, 0);
    IssueL1Prefetches(cycle, L1Data);
}
