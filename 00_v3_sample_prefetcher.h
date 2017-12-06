// Author: Martin Dimitrov
// Paper Title: Data Prefetching Using Local Delta Buffers
// email: dimitrov@cs.ucf.edu
// Date: Dec. 12 2008
// School of Electrical Engineering and Computer Science
// University of Central Florida

#ifndef MY_PREFETCHER_H
#define MY_PREFETCHER_H

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <assert.h>

#include "interface.h"

#define MAX(a,b)    ((a>b)? a : b )
#define MIN(a,b)    ((a>b)? b : a )

#define INVALID_ADDR 0xBADBEEF
#define DELTA_BUF_SIZE 8
#define PREF_BUF_SIZE 8
#define LHB_SIZE 7
#define LHB_DELTA_MIN -8388608	// -2^23
#define LHB_DELTA_MAX 8388608 	// 2^23

using namespace std;

template < class ItemType > class Tag
{
  public:
    ADDRINT tag;
    UINT32 LRU;                 // for LRU replacement
    ItemType *Item;

    Tag()
    {
        tag = INVALID_ADDR;
        Item = new ItemType;
    }

    ~Tag()
    {
        delete Item;
    }

	void dump(){
		cout << "Tag: " <<hex<< tag <<dec;
		cout << " LRU: " << LRU<<endl;
		Item->dump();
	}
};

// This class descibes a generic cache-like table
template < class ItemType > class _CacheLikeTable
{
  public:

    Tag < ItemType > **TagArray;        // tag store
    int num_lines;              // number of lines (may not be a 2's power)
    int num_sets;               // number of sets per line

    _CacheLikeTable(int n_l, int n_s)
    {
        int i, j;

        num_lines = n_l;
        num_sets = n_s;
        TagArray = new Tag < ItemType > *[num_lines];

        for (i = 0; i < num_lines; i++)
        {
            TagArray[i] = new Tag < ItemType >[num_sets];
            for (j = 0; j < num_sets; j++)
            {
                TagArray[i][j].LRU = j;
            }
        }

    }

    ~_CacheLikeTable()
    {
        for (int i = 0; i < num_lines; i++)
        {
            for (int j = 0; j < num_sets; j++)
                if (TagArray[i][j].Item != NULL)
                    delete TagArray[i][j].Item;
            delete TagArray[i];
        }
        delete TagArray;
    }

  private:
    
	// Checking hit or miss does not update the LRU bits
    bool Hit(ADDRINT addr)
    {
        int j;
        int line = addr % num_lines;

        for (j = 0; j < num_sets; j++)
            if (TagArray[line][j].tag == addr)
                return true;
        return false;
    }

  public:
    
	// Access the TagArray and but do NOT update the LRU bits
    // r_Tag: return pointer to the Item (if hit). If miss, return false
    Tag < ItemType > *AccessTag(ADDRINT addr, bool * p_hit)
    {
        int j;
        int line = addr % num_lines;
        bool hit = false;
        int hit_way = -1;

        for (j = 0; j < num_sets; j++)
        {
            if (TagArray[line][j].tag == addr)
            {
                hit = true;
                hit_way = j;
                break;
            }
        }

        if (hit)
        {
            *p_hit = true;
            return (&TagArray[line][hit_way]);
        }
        else
        {
            *p_hit = false;
            return (NULL);
        }
    }


    // Access the TagArray and updates the LRU bits
    // r_Tag: return pointer to the Item (if hit). If miss, points to the LRU
    // entry
    // to be replaced.
    Tag < ItemType > *AccessTagWLRU(ADDRINT addr, bool * p_hit)
    {
        int j;
        int line = addr % num_lines;
        bool hit = false;
        int hit_way = -1;

        for (j = 0; j < num_sets; j++)
        {
            if (TagArray[line][j].tag == addr)
            {
                hit = true;
                hit_way = j;
                break;
            }
        }

        if (hit)
        {
            for (j = 0; j < num_sets; j++)
            {
                if (TagArray[line][j].LRU < TagArray[line][hit_way].LRU)
                    TagArray[line][j].LRU++;
            }
            TagArray[line][hit_way].LRU = 0;
            *p_hit = true;
            return (&TagArray[line][hit_way]);
        }
        else
        {
            int replace_way = -1;

            for (j = 0; j < num_sets; j++)
            {
                if (TagArray[line][j].LRU == (unsigned int)(num_sets - 1))
                {
                    replace_way = j;
                    TagArray[line][j].LRU = 0;
                }
                else
                {
                    TagArray[line][j].LRU += 1;
                }
            }
            *p_hit = false;
            return (&TagArray[line][replace_way]);
        }
    }

    // Access the TagArray and updates the LRU bits
    // r_Tag: return pointer to the Item (if hit). If miss, points to the LRU
    // entry
    // to be replaced.
    Tag < ItemType > *AccessTagWRandom(ADDRINT addr, bool * p_hit)
    {
        int j;
        int line = addr % num_lines;
        bool hit = false;
        int hit_way = -1;

        for (j = 0; j < num_sets; j++)
        {
            if (TagArray[line][j].tag == addr)
            {
                hit = true;
                hit_way = j;
                break;
            }
        }

        if (hit)
        {
            *p_hit = true;
            return (&TagArray[line][hit_way]);
        }
        else
        {
            int replace_way = rand() % num_sets;
            *p_hit = false;
            return (&TagArray[line][replace_way]);
        }
    }

	void dump(){
		cout<<"======================================================"<<endl;
		cout <<"CacheLikeTable: "<<endl;
        for (INT32 i = 0; i < num_lines; i++){
            for (INT32 j = 0; j < num_sets; j++){
                TagArray[i][j].dump();
			}
			cout<<endl;
		}
		cout<<"======================================================"<<endl;
	}

};

// Local History Buffer
// In contrast to a global history buffer (GHB) with the LHB
// each PC keeps it's own address stream history
class LHB
{
  public:

    INT32 lhb[LHB_SIZE]; // the LBH is an array of address deltas
	ADDRINT last_addr; // the last addr accessed by the PC
    INT32 head;	// head of the LHB queue
	bool conf;  // a single bit indicating if we have already prefetched a strong pattern

      LHB()
    {
        for (INT32 i = 0; i < LHB_SIZE; i++)
            lhb[i] = -1;
        head = 0;
		last_addr = INVALID_ADDR;
		conf = false;
    }
    ~LHB()
    {
        delete lhb;
    }

	void reset(){
        for (INT32 i = 0; i < LHB_SIZE; i++)
            lhb[i] = -1;
        head = 0;
		last_addr = INVALID_ADDR;
		conf = false;
	}

	// Add a new delta to the LHB
    void push(ADDRINT m_addr)
    {
		// If no previous history exists in the LHB
		if(INVALID_ADDR == last_addr){
			assert(0 == head);
			last_addr = m_addr;
		}
		else{
			INT32 delta = m_addr - last_addr;
			
			assert((delta > LHB_DELTA_MIN)&&(delta < LHB_DELTA_MAX));
			lhb[head] = delta;
			head = (head + 1) % LHB_SIZE;
			last_addr = m_addr;
		}
    }

	// Get a pointer to the last entry inserted into the LHB
	// The return value is the address last inserted into the LHB
	// (this value may be INVALID_ADDR, if nothing was inserted yet)
	ADDRINT get_last(INT32 *last_index){
		INT32 ind;
		if(0 == head)
			ind = LHB_SIZE-1;
		else
			ind = head-1;
		*last_index = ind;
		return last_addr;
	}

	// Input: a pointer to the current entry in LHB
	// Ouput: a pointer to the next entry in LHB, 
	// 		  the delta contained in the entry pointed to by index
    INT32 get_next(INT32 index, INT32 * next_index)
    {

		INT32 n_index; 

		// Prevent wrap-around
		if(-1 == index){
			*next_index = -1;
			return -1;
		}
		if(index == head){
			*next_index = -1;
			return lhb[index];
		}

		if(0 == index)
			n_index = LHB_SIZE-1;
		else
			n_index = index-1;

		*next_index = n_index;
		return lhb[index];
    }

    // Access lhb: update LHB and returns a buffer with up to 8 deltas
    void access_lhb(ADDRINT m_addr, INT32 * delta_buf, INT32 * delta_buf_size)
    {
        ADDRINT addr = m_addr;
		INT32 last_index;
		ADDRINT l_addr = get_last(&last_index);

		assert(INVALID_ADDR != l_addr);
		
        // Filter addresses which are to the same line, 
		// or delta that are too large. 
		INT32 diff = addr - l_addr;
        if(diff > -64 && diff < 64)
        {
            *delta_buf_size = 0;
            return;
        }
		
		// restrict the delta to only 24 bits
		// If the delta is less than -2^23 
		// or greater than 2^22, then do not insert it in the LHB
		else if(diff <= LHB_DELTA_MIN || diff >= LHB_DELTA_MAX)
		{
            *delta_buf_size = 0;
            return;
		}
        else
        {
			INT32 i=0;
			INT32 ind;
			INT32 n_ind;
            delta_buf[i] = addr - l_addr; // insert the first delta
            i++;

			ind = last_index;

            INT32 next_delta = get_next(ind, &n_ind);
            while (-1 != next_delta && i < DELTA_BUF_SIZE)
            {
                delta_buf[i] = next_delta;
                i++;

                ind = n_ind;
                next_delta = get_next(ind, &n_ind);
            }
            *delta_buf_size = i;

			push(m_addr);
			return;
        }
    }
    void dump()
    {
        cout << "LHB head: " << head << endl;
        for (INT32 i = 0; i < LHB_SIZE; i++)
            cout << "entry " << i << ": delta: " << hex << lhb[i] << endl;
    }
};


// the storage requirement of each prefetch table entry is as follows: 
// 29 bits tag + 3 bits LRU + 1 confidence bit + 32 last address bits +  
// 7 * 24 LDB delta bits + 3 LDB head pointer bits = 236 bits. 
// The storage requirement for the prefetch table is 64 entries * 236 bits = 15,104 bits. 
class LHBPrefetcher:_CacheLikeTable <LHB>
{
  public:
    INT32 num_pref_delta_context;
    INT32 num_pref_delta_context_simple;
    INT32 num_pref_next_line;

  private:


    // Compare two deltas.  
    // Can detect strides such as: 
    // - a b c b, a b c b, a b c b. 
    // - a a b, a a b, a a b
    // - a a a a a a a a a
    // - a b c, a b c, a b c ect.
      INT32 LocalDeltaContext(INT32 d_buf_size, INT32 * d_buf, ADDRINT m_addr,
                              ADDRINT * pref_addr, bool * strong_match)
    {
        if (d_buf_size >= 4)
        {
            int k;
            ADDRINT temp_addr;
            for (k = 2; k < d_buf_size - 1; k++)
            {
				// found a match in delta
                if ((d_buf[k] == d_buf[0]) && (d_buf[k + 1] == d_buf[1]))  
                {

					// use logic to determine if this is a strong match
		    		if(k < d_buf_size - 2)
				      if(d_buf[k+2] == d_buf[2])
						*strong_match = true;

                    int pattern_end = k;
                    temp_addr = m_addr;
                    for (int a = 0; a < PREF_BUF_SIZE ; a++)
                    {
                        temp_addr = temp_addr + d_buf[k - 1];
                        pref_addr[a] = temp_addr;
                        k--;
                        if (0 == k)
                            k = pattern_end;    // start over with the pattern
                    }
                    num_pref_delta_context = num_pref_delta_context + PREF_BUF_SIZE;

                    return PREF_BUF_SIZE;
                }
            }
        }
        return 0;
    }                           // LocalDeltaContext
      
    // The current stride matches a previously observed stride
    // This may be the immediate neighbor or it may be several 
    // deltas away. In either case, we assume that the pattern 
    // between [0] and [k] is stable so we start prefetching
    // this pattern, backwards (from k-1 to 0)
    INT32 LocalDeltaContextSimple(INT32 d_buf_size, INT32 * d_buf,
                                  ADDRINT m_addr, ADDRINT * pref_addr)
    {
        INT32 pref_degree = PREF_BUF_SIZE/2;

        if (d_buf_size >= 2)
        {
            INT32 k;
            ADDRINT temp_addr;

            for (k = 1; k < d_buf_size; k++)
            {
                if (d_buf[k] == d_buf[0])       // found a match in delta
                {
                    INT32 pattern_end = k;

                    temp_addr = m_addr;

                    for (INT32 a = 0; a < pref_degree; a++)
                    {

                        temp_addr = temp_addr + d_buf[k - 1];
                        pref_addr[a] = temp_addr;
                        k--;

                        if (0 == k)
                            k = pattern_end;    // start over with the pattern
                    }
                    num_pref_delta_context_simple = num_pref_delta_context_simple + pref_degree;
                    return pref_degree;
                }
            }
        }
        return 0;
    }                           // LocalDeltaContextSimple


  public:
    
	// index_table_size specifies how many entrys are in the index table
    // index_table_asso specifies how the entries are organized (set associativity)
	LHBPrefetcher(INT32 index_table_size, INT32 index_table_asso):_CacheLikeTable <LHB >
  				::_CacheLikeTable(index_table_size/index_table_asso,index_table_asso)
    {
    	num_pref_delta_context=0;
	    num_pref_delta_context_simple=0;
    	num_pref_next_line=0;
    }
    ~LHBPrefetcher()
    {
    }

    void PrintStats()
    {
        cout << "number of prefetches:";
		cout << " delta_context: " << num_pref_delta_context;
        cout << " delta_context simple: " << num_pref_delta_context_simple;
        cout << " next_line: " << num_pref_next_line<<endl;
    }


    // Access Prefetcher
    // returns a number of prefetch addresses
    // the number (up to 8) of prefetch addresses is the returned value
    INT32 AccessPrefetcher(ADDRINT m_addr, ADDRINT ip, ADDRINT * pref_addr)
    {
        Tag <LHB> *t;
        bool found;
        INT32 d_buf[DELTA_BUF_SIZE];
        INT32 d_buf_size;
        INT32 num_prefetches = 0;

//        t = AccessTagWRandom(ip, &found);
        t = AccessTagWLRU(ip, &found);
        assert(t != NULL);
        LHB *lhb = t->Item;

        if (found == false)
        {
            // This PC has missed in the first level table.  
			// claim the LHB for this new PC
      		t->tag = ip;
			lhb->reset();
			lhb->push(m_addr);
            return 0;
        }
        else
        {
            lhb->access_lhb(m_addr, d_buf, &d_buf_size);

			// Prefetch based on the un-sorted stream
            if (d_buf_size > 0)
            {

				bool strong_match = false;

                // Local delta context
                num_prefetches = LocalDeltaContext(d_buf_size, d_buf, m_addr, pref_addr, &strong_match);

                // Local delta context simple
                if (0 == num_prefetches)
                {
                    num_prefetches += LocalDeltaContextSimple(d_buf_size, d_buf, m_addr,pref_addr);
                }

				if(0 == num_prefetches)
				{
					pref_addr[0] = (m_addr & 0xffffffc0) + 64;
					num_prefetches = 1;
					num_pref_next_line++;
				}

				// Use logic to filter out redundant prefetches
	  			if(num_prefetches == PREF_BUF_SIZE) 
	  			{			

					// If this is a pure constant stride, or if it is a strong delta-context match
					// prefetch only the last prefetch of the 8. The previous 7 should have already been 
					// prefetched on the previous access. 
	    			if(((pref_addr[0] == m_addr + d_buf[0]) && 
					    (pref_addr[3] == m_addr + 4 * d_buf[0]) && 
					    (pref_addr[7] == m_addr + 8 * d_buf[0])
					   )|| strong_match)
	    			{
				    	if(lhb->conf == true){
							pref_addr[0] = pref_addr[PREF_BUF_SIZE - 1];
							num_prefetches = 1;
	      				}
					    else{
							lhb->conf = true;
						}
	    			}
	    			else{
						lhb->conf = false;
	    			}
	  			}
	  			else{
	    			lhb->conf = false;
	    		}


            }
			else
			{
				assert(0 == d_buf_size);
			}
			

            assert(PREF_BUF_SIZE >= num_prefetches);
            return num_prefetches;
        }
    }


    void dump()
    {
        cout << "Index table:" << endl;
		_CacheLikeTable <LHB> :: dump();
    }
};

// A MSHR: 
// We assume that the storage requirement for the MSHR is not
// counted towards our budget. (we use a 16 entry MSHR)
class MSHR:_CacheLikeTable < ADDRINT >
{
  public:
    UINT32 num_conflicts;

	MSHR(UINT32 tableSize):_CacheLikeTable < ADDRINT > (1, tableSize)
    {	
		num_conflicts=0;
    }

    MSHR(UINT32 sets, UINT32 assoc):_CacheLikeTable < ADDRINT > (sets, assoc)
    {
		num_conflicts=0;
    }
     ~MSHR()
    {
    };

    // Look-up in the mshr table. This leads to a subsequent LRU update in the 
    // table. Returns 1 if hit and 0 if miss.
    bool AccessEntry(ADDRINT memAddr)
    {
		ADDRINT addr = memAddr >> 6;
        Tag < ADDRINT > *t;
        bool found;

        t = AccessTagWLRU(addr, &found);
        if (found == false){
			if(INVALID_ADDR != t->tag)
				num_conflicts++;
            t->tag = addr;
		}
        return (found);
    }

    bool Hit(ADDRINT memAddr)
    {
		ADDRINT addr = memAddr >> 6;
        bool found;

        Tag < ADDRINT > *t;
        t = AccessTag(addr, &found);
        return (found);
    }

    bool HitWLRU(ADDRINT memAddr)
    {
        bool found;
		ADDRINT addr = memAddr >> 6;

        Tag < ADDRINT > *t;
        t = AccessTagWLRU(addr, &found);
        return (found);
    }

    // Scan the MSHR and test the prefetch bit for each address. If the
    // prefetch bit
    // is not -1, that means that the addr is already in the cache and we can 
    // release the MSHR entry. Set the address of the released entry to
    // invalid and move it to the tail of the queue (LRU). 
    void Release(COUNTER cycle, UINT32 id)
    {
        INT32 pref_bit;

        for (int i = 0; i < num_lines; i++)
        {
            for (int j = 0; j < num_sets; j++)
			{
				ADDRINT addr = (TagArray[i][j].tag)<<6;
				pref_bit = GetPrefetchBit(id, addr);
            	if (pref_bit != -1) // already in the cache
	            {
					// Invalidate the entry and update the LRU bits
            	    TagArray[i][j].tag = INVALID_ADDR;
	               	for (int k = 0; k < num_sets; k++)
    	            {
        	            if (TagArray[i][k].LRU > TagArray[i][j].LRU)
            	            TagArray[i][k].LRU--;
                	}
	                TagArray[i][j].LRU = num_sets - 1;
    	        }
			}
		}
	}

	void dump(){
		cout<<"======================================================"<<endl;
		cout<<"MSHR entries: "<<num_lines * num_sets<<endl;
		cout<<"num conflicts: "<<num_conflicts<<endl;
        for (INT32 i = 0; i < num_lines; i++){
            for (INT32 j = 0; j < num_sets; j++){
                cout <<"["<<i<<"]["<<j<<"] "<<hex<<TagArray[i][j].tag<<" ";
			}
			cout<<endl;
		}
		cout<<"======================================================"<<endl;
	}
};


// The storage requirement of the Bloom filter
// is simply the number of entries. We assume 4K entries. 
class BloomFilter
{
	private:
	
	UINT32 *array; // the bloom filter array
	INT32 num_entries;
	INT32 shift_amount;

  	public:

    BloomFilter(UINT32 n_entries)
    {
		num_entries = n_entries; 
		shift_amount = log2(num_entries);
		array = new UINT32 [num_entries];
		flush();
    }

     ~BloomFilter()
    {
		if(array)
			delete array;
    };

	/* Wikipedia code for computing log base 2 */
	INT32 log2(INT32 x){
		INT32 r=0;
		while( (x >> r) != 0){
			r++;
		}
		return r-1; // returns -1 for x==0, floor(log2(x)) otherwise
	}

	void flush(){
		for(INT32 i=0; i<num_entries; i++){
			array[i] =0;
		}
	}

	void Update(ADDRINT addr){
		UINT32 bit_addr = addr & 0xffffffff;
		UINT32 xor_addr = (bit_addr >> shift_amount) ^ bit_addr;
		INT32 index = xor_addr % num_entries;
		array[index] = 1;
	}
	
	UINT32 Hit(ADDRINT addr){
		UINT32 bit_addr = addr & 0xffffffff;
		UINT32 xor_addr = (bit_addr >> shift_amount) ^ bit_addr;
		INT32 index = xor_addr % num_entries;
		return array[index];
	}

};

class SamplePrefetcher:public LHBPrefetcher
{
  private:

    // Statistics
    UINT32 num_prefetches;      // number of issued prefetches
    UINT32 num_partial_prefetches;      // number of issued prefetches
    UINT32 num_good_prefetches; // number of prefetches followed by a hit

  public:

    SamplePrefetcher():LHBPrefetcher(64, 8)
    {
        num_prefetches = 0;
        num_partial_prefetches = 0;
        num_good_prefetches = 0;
    }

    void inc_num_prefetches()
    {
        num_prefetches++;
    }

    void inc_num_good_prefetches()
    {
        num_good_prefetches++;
    }

    void inc_num_partial_prefetches()
    {
        num_partial_prefetches++;
    }

    void PrintStats()
    {
        LHBPrefetcher::PrintStats();
        cout << "num_prefetches: " << num_prefetches;
        cout << " num_partial_prefetches: " << num_partial_prefetches;
        cout << " num_good_prefetches: " << num_good_prefetches << endl;
    }

    ~SamplePrefetcher()
    {
    }
};

#endif
