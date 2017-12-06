#include "interface.h"          // Do NOT edit interface .h

#include "sample_prefetcher.h"
#include <assert.h>

SamplePrefetcher *pref1;        // This is the L1 prefetcher

MSHR *mshr1;                    // This is the L1 MSHR

BloomFilter *filter1;
UINT32 num_filter_access = 0;

// Global statistics
UINT32 num_cache_hit_l1 = 0;
UINT32 num_cache_miss_l1 = 0;

UINT32 num_cache_hit_l2 = 0;
UINT32 num_cache_miss_l2 = 0;

//Adaptive turn-on or off the prefetcher
bool enable_prefetch = true;
UINT32 phase_count = 0;
UINT32 num_l2_cache_miss_reg = 0;
bool training = false;

void inc_num_cache_hit_l1(ADDRINT addr)
{
    num_cache_hit_l1++;
}

void inc_num_cache_hit_l2(ADDRINT addr)
{
    num_cache_hit_l2++;
}

void inc_num_cache_miss_l1(ADDRINT addr)
{
    num_cache_miss_l1++;
}

void inc_num_cache_miss_l2(ADDRINT addr)
{
    num_cache_miss_l2++;
}


// 
// Function to initialize the prefetchers.  DO NOT change the prototype of
// this
// function.  You can change the body of the function by calling your
// necessary
// initialization functions.
// 

void InitPrefetchers()          // DO NOT CHANGE THE PROTOTYPE
{
    // INSERT YOUR CHANGES IN HERE

    pref1 = new SamplePrefetcher();
   
    mshr1 = new MSHR(16, 16);

    filter1 = new BloomFilter(2048);    
}



void IssueL1Prefetches(const COUNTER cycle, const PrefetchData_t * L1Data)
{

    // Issue L1 prefetches
    // Up to four records can be given in a single cycle depending on the
    // number of requests
    // (up to four) issued to the L1 data cache
  
	
    INT32 k;

    INT32 pref_bit;
    UINT32 mshr_hit;
    UINT32 pmshr_hit;
    UINT32 queue_full;

    for(INT32 req = 0; req < 4; req++){

      ADDRINT pref_index = L1Data[req].LastRequestAddr;
 
      UINT32 pref1_hit = GetPrefetchBit(0, L1Data[req].DataAddr & 0xffffffc0);
    // Issue L1 prefetches
      if ((cycle == L1Data[req].LastRequestCycle) && ((L1Data[req].hit == 0) || (pref1_hit)))
      {
	if (L1Data[req].hit != 0){
	  mshr_hit = mshr1->Hit((L1Data[req].DataAddr & 0xffffffc0) >> 6);
	}
	else{
	  mshr_hit = mshr1->AccessEntry((L1Data[req].DataAddr & 0xffffffc0) >> 6);
	}
	if (0 == mshr_hit)
	{
	  ADDRINT pref_addr[PREF_BUF_SIZE];
	  INT32 pref_degree;
	  INT32 n_pref_DC = 0;        // Number of predictions from Delta Context
	  INT32 n_pref_DC_s = 0;      // Number of prediction from Delta Context simple
	  INT32 n_pref_GS = 0;        // Number of predictions from Global Stride
	  INT32 partial = 0; 			// Is this miss the partially overlapped
	  
	  pref_degree = pref1->AccessPrefetcher(L1Data[req].DataAddr, pref_index, pref_addr, &n_pref_DC, &n_pref_DC_s, &n_pref_GS);   
	  
	  // Collect some stats
	  if (L1Data[req].hit == 0)
	  {
	    inc_num_cache_miss_l1(L1Data[req].DataAddr);        // & 0xffffffc0);
	    pmshr_hit = filter1->Hit((L1Data[req].DataAddr & 0xffffffc0) >> 6); 
	    if (pmshr_hit)
	    {
	      partial = 1; 
	      pref1->inc_num_partial_prefetches();
	    }
	  }
	  else
	  {
	    inc_num_cache_hit_l1(L1Data[req].DataAddr & 0xffffffc0);
	    pref_bit = GetPrefetchBit(0, L1Data[req].DataAddr & 0xffffffc0);
	    
	    if (pref_bit == 1)
	    {
	      UnSetPrefetchBit(0, L1Data[req].DataAddr & 0xffffffc0);
	      pref1->inc_num_good_prefetches();
	    }
	  }
	  for (k = 0; k < pref_degree; k++)
	  {
	    if(training == true)
	    {
	      if(((pref_addr[k] >> 6) & 3) == 2)
		continue;
	    }
	    pmshr_hit = filter1->Hit((pref_addr[k] & 0xffffffc0) >> 6); 
	        
	    if ((0 == pmshr_hit) && (enable_prefetch == true))
	    {
	      queue_full = IssueL1Prefetch(cycle, pref_addr[k]);
	      if (0 == queue_full)
	      {           // if prefetch is not dropped
		filter1->Update((pref_addr[k] & 0xffffffc0) >> 6);
		num_filter_access++;
		if(num_filter_access > 256){
		  num_filter_access=0;				
		  filter1->flush(); 
		}
		pref1->inc_num_prefetches();
	      }
	      
	    }
	  }
	}                       // mshr miss
      }
      else if ((cycle == L1Data[req].LastRequestCycle) && (L1Data[req].hit == 1))
      {
        inc_num_cache_hit_l1(L1Data[req].DataAddr & 0xffffffc0);
      }
      
    }//for each request
}
//No prefetches issued here, just collect some stats
void IssueL2Prefetches(const COUNTER cycle, const PrefetchData_t * L2Data)
{
    if ((cycle == L2Data->LastRequestCycle) && (L2Data->hit == 0))   
    {
      // Collect some stats            
      if(L2Data->AccessType == 1 | L2Data->AccessType ==2)
      {
	inc_num_cache_miss_l2(L2Data->DataAddr);        // & 0xffffffc0);
	if(((L2Data->DataAddr >> 6) & 3) == 2) num_l2_cache_miss_reg++;
      }      
    }    
}
// 
// Function that is called every cycle to issue prefetches should the
// prefetcher want to.  The arguments to the function are the current cycle,
// the demand requests to L1 and L2 cache.  Again, DO NOT change the prototype 
// of this
// function.  You can change the body of the function by calling your
// necessary
// routines to invoke your prefetcher.
// 

// DO NOT CHANGE THE PROTOTYPE
void IssuePrefetches(COUNTER cycle, PrefetchData_t * L1Data, PrefetchData_t * L2Data)
{
    // INSERT YOUR CHANGES IN HERE

    // Release MSHR entries
    mshr1->Release(cycle, 0);

    IssueL1Prefetches(cycle, L1Data);
    IssueL2Prefetches(cycle, L2Data); //only used for collecting L2 miss stats

    // every 1000000 cycles
    if (0 == cycle % 1000000)
    {
      //pref1->PrintStats();
      phase_count++;
      if(enable_prefetch == false)
      {
	if(num_cache_miss_l1 > 25000)// if a high number of misses, turn on prefetcher more aggressively
	  phase_count = 5;
      }
      if(cycle >= 1000000) 
	training = true;
      if(enable_prefetch == true)
      {
	if(training)
	{
	  if(((num_l2_cache_miss_reg * 6) < num_cache_miss_l2) && (num_cache_miss_l2 > 256) )
	    {
	      enable_prefetch = false;
	    }
	  if(enable_prefetch == false) phase_count = 0;
	}
      }
	
      if(training == true && enable_prefetch == true)
      {	 
	if(pref1->get_num_prefetches() == 0)
	  training = false;
	else
	{
	  //need to consider both coverage and accuracy
	  int accu = (pref1->get_num_good_prefetches()+ pref1->get_num_partial_prefetches()) * 1000 / pref1->get_num_prefetches();
	  int coverage = pref1->get_num_good_prefetches()+ pref1->get_num_partial_prefetches();
	  //successful prefetches turn misses into hits
	  if(num_cache_miss_l1 + pref1->get_num_good_prefetches() < 2500)
	  {
	    enable_prefetch = false;
	    phase_count = 4;	      
	  }
	  else if(num_cache_miss_l1 + pref1->get_num_good_prefetches() < 15000)
	  {
	    //require accuracy & coverage
	    if(accu > 250 && coverage > 5000 )
	    {
	      training = false;	      
	    }	    
	  }
	  else 
	  {
	    if((accu > 125 && coverage > 10000) || (accu > 250 && coverage > 5000)) 
	    {
	      training = false;
	    }	    
	  }
	}
      }
      if(phase_count == 5)
      {
	enable_prefetch = true;
	phase_count = 0;
      }
	
      pref1->reset_stats();
      
      num_cache_miss_l1 = 0;
      num_cache_miss_l2 = 0;
      num_l2_cache_miss_reg = 0;
	
    }
}
