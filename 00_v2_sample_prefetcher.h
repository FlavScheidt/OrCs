//Prefetcher header file.
//Title: Combining Local and Global History for High Perfrormance Data Prefetching 
//Version 2: Using a Bloom filter to filter out redundant prefetches  
//Author: Martin Dimitrov and Huiyang Zhou 
//School of Electrical Engineering and Computer Science
//University of Central Florida

#ifndef MY_PREFETCHER_H
#define MY_PREFETCHER_H

#include <stdio.h>
#include <fstream>
#include <iostream>

#include "interface.h" 

#define MAX(a,b)    ((a>b)? a : b )
#define MIN(a,b)    ((a>b)? b : a )

#define INVALID_ADDR 0xBADBEEF
#define DELTA_BUF_SIZE 32
#define PREF_BUF_SIZE 8 //Max prefetch degree
#define LDB_SIZE 8 //Although the size here is specified as 8, the lastest delta is calculated using logic. therefore, only 7 entries are accounted for the storage cost

#define LDB_NUM 16

using namespace std;

template<class ItemType>
class Tag{
 public: 
	ADDRINT tag; 
	UINT32 LRU; //for LRU replacement
	ItemType * Item; 
	
	Tag(){
	tag = INVALID_ADDR;
	Item = new ItemType;
	}
	
	~Tag(){
	delete Item;
	}
};

class IndexItem
{
 public:
	INT32 index;
};

template<class ItemType>
class _CacheLikeTable
{
 public:

	Tag<ItemType> **TagArray; //tag store
	int num_lines; //number of lines (may not be a 2's power)
	int num_sets; //number of sets per line
	
	_CacheLikeTable(int n_l, int n_s)
	{
	int i,j;
	num_lines = n_l;

	num_sets = n_s;
	TagArray = new Tag<ItemType> * [num_lines];
	
	for(i =0; i < num_lines; i++)
	{
		TagArray[i] = new Tag<ItemType>[num_sets];      
		for(j = 0; j < num_sets; j++)
		{
		TagArray[i][j].LRU = j;
		}
	}    

	}
	~_CacheLikeTable()
	{
	for(int i =0; i < num_lines; i++)
	{
		for(int j = 0; j < num_sets; j++)
	if(TagArray[i][j].Item != NULL) 
		delete TagArray[i][j].Item;
		delete TagArray[i];
	}
	delete TagArray;
	}
 private:
	
	//Checking hit or miss does not update the LRU bits
	bool Hit(ADDRINT addr)
	{
	int j;
	int line = addr % num_lines;
	for(j = 0; j < num_sets; j++)
		if(TagArray[line][j].tag == addr)
	return true;
	return false;
	}
	//m_tag may be a hashfunction of addr
	bool Hit(ADDRINT addr, ADDRINT m_tag)
	{
	int j;
	int line = addr % num_lines;
	for(j = 0; j < num_sets; j++)
		if(TagArray[line][j].tag == m_tag)
	return true;
	return false;
	}
 public:
	//Access the TagArray and but do  NOT update the LRU bits
	//r_Tag: return pointer to the Item (if hit). If miss, return false
	Tag<ItemType> * AccessTag(ADDRINT addr, bool * p_hit)
	{
	int j;
	int line = addr % num_lines;
	bool hit = false;
	int hit_way = -1;
	for(j = 0; j < num_sets; j++)
	{
		if(TagArray[line][j].tag == addr)
		{
	hit = true;
	hit_way = j;
	break;
		}
	}

	if(hit)
	{
		*p_hit = true;
		return(& TagArray[line][hit_way]);
	}
	else
	{
		*p_hit = false;
		return (NULL);      
	}
	}  


	//Access the TagArray and updates the LRU bits
	//r_Tag: return pointer to the Item (if hit). If miss, points to the LRU entry
	// to be replaced.
	Tag<ItemType> *  AccessTagWLRU(ADDRINT addr, bool * p_hit)
	{
	int j;
	int line = addr % num_lines;
	bool hit = false;
	int hit_way = -1;
	for(j = 0; j < num_sets; j++)
	{
		if(TagArray[line][j].tag == addr)
		{
	hit = true;
	hit_way = j;
	break;
		}
	}

	if(hit)
	{
		for(j = 0; j < num_sets; j++)
		{
	if(TagArray[line][j].LRU < TagArray[line][hit_way].LRU)
		TagArray[line][j].LRU++;
		}
		TagArray[line][hit_way].LRU = 0;
		*p_hit = true;
		return (& TagArray[line][hit_way]);
	}
	else
	{
		int replace_way = -1;
		for (j = 0; j < num_sets; j++) 
		{
	if (TagArray[line][j].LRU == (unsigned int)(num_sets-1)) 
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
		return( &TagArray[line][replace_way]);
	}
	}

};

class GHB
{
 public:
	
	typedef struct _ghb_entry
	{
		ADDRINT mem_addr; 
		int next; //points to next entry with the same index (from index table)
	} ghb_entry;

	ghb_entry * ghb;
	INT32 size;
	INT32 head;
	
	GHB(int s)
	{
		size = s;
		ghb = new ghb_entry[size];
		for(int i = 0; i < size; i++)
		{
			ghb[i].mem_addr = INVALID_ADDR;
			ghb[i].next = -1;
		}
		head = 0;
	}
	~GHB()
	{
		delete ghb;
	}

	INT32 push(ADDRINT m_addr, INT32 last_index)
	{
		INT32 index = head;
		ghb[head].mem_addr = m_addr;
		ghb[head].next = last_index;
		head = (head + 1) % size;
		return(index);
	}
	//give the current position of the head pointer, determine whether the
	//first entry is older than the second (i.e., whether the first miss occurred 
	// earlier than the second)
	bool older(INT32 ind1, INT32 ind2)
	{
		if(ind1 == ind2)
		{
			return false;
		}
		INT32 oldest = head;
		INT32 diff1 = oldest - ind1;
		if(diff1 < 0)
			diff1 += size;
		INT32 diff2 = oldest - ind2;
		if(diff2 < 0)
			diff2 += size;
		if(diff1 > diff2)
			return true;
		else
			return false;
	}
	//the return pointer points to ghb[ghb[index].next], and next_index is set to ghb[index].next 
	ghb_entry * get_next(INT32 index, INT32 * next_index)
	{
		if(index == -1)
			return(NULL);
		INT32 n_index = ghb[index].next;
		*next_index = n_index;
		if(n_index == -1)
			return NULL;
		if(older(n_index, index))
			return (&ghb[n_index]);
		else
			return NULL;
	}
	
	//Access ghb: update GHB and returns a buffer with up to 8 deltas
	//the return value tells the new index
	INT32 access_ghb(ADDRINT m_addr, INT32 last_index, INT32 *delta_buf, INT32 *delta_buf_size)
	{
		ADDRINT addr = m_addr;
		INT32 ind = last_index;
		INT32 n_ind = ind;
		INT32 i = 0;

		
		INT32 diff = m_addr - ghb[last_index].mem_addr;
		if(diff > -64 && diff < 64)
		{
			*delta_buf_size = 0;
			return last_index;
		}
		else
		{      
			delta_buf[i] = addr - ghb[last_index].mem_addr;
			i++;
			addr = ghb[last_index].mem_addr;
			ind = last_index;
			
			ghb_entry * g_next = get_next(ind, &n_ind);
			while(g_next != NULL && i < DELTA_BUF_SIZE)
			{
			delta_buf[i] = addr - g_next->mem_addr;
			i++;
			addr = g_next->mem_addr;
			ind = n_ind;
			g_next = get_next(ind, &n_ind);
			}
			*delta_buf_size = i;
			return push(m_addr, last_index);
		}
	}
	void dump()
	{
	cout << "head: " << head << endl;
	for(int i = 0; i < size; i++)
		cout << "entry " << i << ": addr: " << hex << ghb[i].mem_addr << dec << " next: " << ghb[i].next << endl; 
	}
};


class _Delta_Buffer
{
 public:
	ADDRINT last_addr;
	ADDRINT ip;
	INT32 lru;
	INT32 * delta;
	INT32 last_c_stride;
	INT32 size;
	bool stride; //pure stride pattern (used as confidence)
	_Delta_Buffer(INT32 d_size)
	{
	stride = false;
	size = d_size;
	delta = new INT32[size];
	}
	~_Delta_Buffer() {delete delta;}
	void copy(INT32 * d_buf, INT32 d_size)
	{
		for(int i = 0; i < d_size; i++)
			delta[i] = d_buf[i];
	}
	void push(INT32 d_val)
	{
	for(int i = size - 1; i >= 1; i--)
		delta[i] = delta[i - 1];
	delta[0] = d_val;
	}
	INT32 * get_delta() {return delta;}
	INT32 get_size() {return size;}
	void set_LRU(INT32 LRU){lru = LRU;}
	INT32 get_LRU() {return lru;}
	void set_ip(ADDRINT pc) {ip = pc;}
	ADDRINT get_ip() {return ip;}
	void set_last_addr(ADDRINT addr) {last_addr = addr;}
	ADDRINT get_last_addr() {return last_addr;}
	INT32 get_last_c_stride() { return last_c_stride;}
	void set_last_c_stride(INT32 c_stride) { last_c_stride = c_stride;}
	void set_stride(bool t) {stride = t;}
	bool get_stride() {return stride;}
};

class _Delta_Buffers 
{
 public: 
	_Delta_Buffer ** d_buffers;
	int num_buffers;
 _Delta_Buffers(int num_buf, int buf_size)
	{
	num_buffers = num_buf;
	d_buffers = new _Delta_Buffer * [num_buffers];
	for(int i = 0; i < num_buffers; i++)
	{
		d_buffers[i] = new _Delta_Buffer(buf_size);
		d_buffers[i]->set_LRU (i);
		d_buffers[i]->set_ip(0xDEADBEEF);
	}    
	}
	~_Delta_Buffers() 
	{
	for(int i = 0; i < num_buffers; i++)
		delete d_buffers[i];
	delete d_buffers;
	}
	_Delta_Buffer * get_i(int i) //get the ith delta buffer
	{
	//assert(i >=0 && i < num_buffers);
	return d_buffers[i];
	}
	void dump_LRU()
	{
	for(int i = 0; i < num_buffers; i++)
		cout << "ip: " << hex << d_buffers[i]->get_ip() << dec << "LRU: " << d_buffers[i]->get_LRU() << endl;
	}
	int get_LRU_buf() //get the index of the least recently used delta buf
	{
	for(int i = 0; i < num_buffers; i++)
	{
		if(d_buffers[i]->get_LRU() == num_buffers - 1)
	return i;
	}
	return 0;
	}
	void update_LRU(int index)
	{
	//assert(index >= 0 && index < num_buffers);
	for(int j = 0; j < num_buffers; j++)
		{
	if(d_buffers[j]->get_LRU() < d_buffers[index]->get_LRU())
		d_buffers[j]->set_LRU(d_buffers[j]->get_LRU() + 1);
		}
		d_buffers[index]->set_LRU(0); 
	}
	bool access_d_buf(ADDRINT ip, int & index)
	{
		bool found = false;
		for(int i = 0; i < num_buffers; i++)
		{
			if(d_buffers[i]->get_ip() == ip)
			{
				found = true;
				index = i;
			}
		}
		if(found == true)
		{
			//update LRU
			for(int j = 0; j < num_buffers; j++)
			{
				if(d_buffers[j]->get_LRU() < d_buffers[index]->get_LRU())
					d_buffers[j]->set_LRU(d_buffers[j]->get_LRU() + 1);
			}
			d_buffers[index]->set_LRU(0);
		}
		return (found);
	}
	//increase all the LRU bit
	void increase_LRU()
	{
	for(int i = 0; i < num_buffers; i++)
	{
		d_buffers[i]->set_LRU(d_buffers[i]->get_LRU() + 1);
	}
	}
};

class GHBPrefetcher : _CacheLikeTable<IndexItem>
{
 public:
	int num_pref_l_stride;
	int num_pref_l_dc;
	int num_pref_l_scalar;
	int num_pref_g_stride;
	int num_prefetches_from_delta;
	GHB * ghb_buf;
	int ghb_size;

	_Delta_Buffers *d_bufs;

 private: 


// same as the one below, but try to compare two deltas.  
// Can detect strides such as: 
// - a b c b, a b c b, a b c b. 
// - a a b, a a b, a a b
// - a a a a a a a a a
// - a b c, a b c, a b c  ect.
	INT32 LocalDeltaContext(INT32 d_buf_size, INT32 *d_buf, ADDRINT m_addr, ADDRINT *pref_addr, bool * strong_match){

	*strong_match = false;

	if(d_buf_size >= 4) 
	{
		int k;
		ADDRINT temp_addr;
		for(k = 2; k < d_buf_size-1; k++)
		{
	if((d_buf[k] == d_buf[0]) && (d_buf[k+1] == d_buf[1])) // found a match in delta
	{
		if(k < d_buf_size - 2)
		if(d_buf[k+2] == d_buf[2])
			*strong_match = true;
		int pattern_end = k;
		int num_pref = PREF_BUF_SIZE;
		
		temp_addr = m_addr;
		for(int a = 0; a < num_pref; a++){
		temp_addr = temp_addr + d_buf[k-1];
		pref_addr[a] = temp_addr;
		k--;
		if(0 == k) k=pattern_end; // start over with the pattern
		}
		num_pref_l_dc = num_pref_l_dc + num_pref;
		return num_pref;
	}	
		}
	}
	return 0;
	}// LocalDeltaContext

	// The current stride matches a previously observed stride
	// This may be the immediate neighbor or it may be several 
	// deltas away. In either case, we assume that the pattern 
	// between [0] and [k] is stable so we start prefetching
	// this pattern, backwards (from k-1 to 0)
	INT32 LocalDeltaContextSimple(INT32 d_buf_size, INT32 *d_buf, ADDRINT m_addr, ADDRINT *pref_addr){

	INT32 pref_degree = 4;
		
	if(d_buf_size >= 2) 
	{
		INT32 k;
		ADDRINT temp_addr;
		
		for(k = 1; k < d_buf_size; k++)
		{
	if(d_buf[k] == d_buf[0]) // found a match in delta
	{
		INT32 pattern_end = k;
		temp_addr = m_addr;
		
		for(INT32 a = 0; a < pref_degree; a++){
		
		temp_addr = temp_addr + d_buf[k-1];
		pref_addr[a] = temp_addr;
		k--;
		
		if(0 == k) k=pattern_end; // start over with the pattern
		}
		num_pref_l_stride = num_pref_l_stride + pref_degree;
		return pref_degree;
	}	
		}
	}
	return 0;
	}// LocalDeltaContextSimple

	// Global stride gives us access to the global cache miss stream. 
	// If PC1 misses and at some point later PC2 misses and the stride between
	// the PC1 miss and the PC2 miss is constant - then whenever PC1 misses we can prefetch
	// for PC2. 
	// If "index" points to the GHB entry of PC1 then "index+1" is the cache misse immediately 
	// following PC1. 
	INT32 GlobalStride(INT32 entry_index, ADDRINT m_addr, 				ADDRINT *pref_addr, INT32 next_pref_slot){
	
	INT32 g_index_1; // pointer to the first(younger) GHB entry of PC1
	INT32 g_index_2; // pointer to the second(older) GHB entry of PC1
	INT32 g_stride_1;
	INT32 g_stride_2;
	INT32 g_count = 0;
	INT32 n_misses_between=0; // number of global misses between index1 and index2
	ADDRINT p_addr = INVALID_ADDR;
	
	g_index_1 = ghb_buf->ghb[entry_index].next;
	if(-1 == g_index_1)
		return 0;
	g_index_2 = ghb_buf->ghb[g_index_1].next;
	if(-1 == g_index_2)
		return 0;
	if(false == ghb_buf->older(g_index_2, g_index_1))
		return 0;
	
	n_misses_between = g_index_1 - g_index_2;
	if(n_misses_between < 0)
		n_misses_between = n_misses_between + ghb_size;
	n_misses_between--;
	
	// Loop backwards, so that we put the misses that are further away in the buffer first
	// This is because global stride tends to prefetch misses which are close to each 
	// other in time and does not overlap latency very well
	for(INT32 i=(n_misses_between-1); i>=0; i--){    
		g_stride_1 = ghb_buf->ghb[(g_index_1 + i +1) % ghb_buf->size].mem_addr - ghb_buf->ghb[g_index_1].mem_addr;
		g_stride_2 = ghb_buf->ghb[(g_index_2 + i +1) % ghb_buf->size].mem_addr - ghb_buf->ghb[g_index_2].mem_addr;
		
		if(g_stride_1 == g_stride_2){
	p_addr = (m_addr + g_stride_1) & 0xffffffc0;
	if(p_addr != (m_addr & 0xffffffc0)){ 
		//assert(next_pref_slot < PREF_BUF_SIZE);
		pref_addr[next_pref_slot] = p_addr;
		num_pref_g_stride++;
		g_count++;
		next_pref_slot++;
		if(PREF_BUF_SIZE == next_pref_slot) 
		break; // reached the capacity of the pref_addr buffer
	}
		}
	}
	return g_count;
	}// Global Stride


 public:
	//index_table_size specifies how many entrys are in the index table
	//index_table_asso specifies how the entries are organized (set associativity)
	//GHB_buf_size specifies the size of GHB buffer
	GHBPrefetcher(int index_table_size, int index_table_asso, int GHB_buf_size):_CacheLikeTable<IndexItem>::_CacheLikeTable( index_table_size/index_table_asso, index_table_asso)
	{
	ghb_buf = new GHB(GHB_buf_size);
	ghb_size = GHB_buf_size;
	num_pref_l_stride = 0;
	num_pref_l_dc = 0;
	num_pref_l_scalar = 0 ;
	num_pref_g_stride = 0;  
	d_bufs = new _Delta_Buffers(LDB_NUM, LDB_SIZE);
	num_prefetches_from_delta = 0;
	}
	~GHBPrefetcher()
	{
	for(int i = 0; i < num_lines; i++)
	 for(int j = 0; j < num_sets; j++)
		 delete TagArray[i][j].Item;
	delete ghb_buf;
	delete d_bufs;
	}

	void PrintStats()
	{
	cout << "number of prefetches with local stride: " << num_pref_l_stride;
	cout << " local dc: " << num_pref_l_dc;
	cout << " local scalar: " << num_pref_l_scalar;
	cout << " global stride: " << num_pref_g_stride << endl;
	cout << "num prefetches from delta buffer: " << num_prefetches_from_delta << endl;
	}


	//Access Prefetcher
	//returns a number of prefetch addresses (using 64-byte block address)
	// the number (up to 8) of prefetch addresses is the returned value
//  INT32 AccessPrefetcher(ADDRINT m_addr, ADDRINT ip, ADDRINT * pref_addr)
	INT32 AccessPrefetcher(ADDRINT m_addr, ADDRINT ip, ADDRINT * pref_addr, 
			INT32 *n_pref_DC, INT32 *n_pref_DC_s, INT32 *n_pref_GS)
	{
		Tag<IndexItem> * t;
		bool found;
		INT32 d_buf[DELTA_BUF_SIZE];
		INT32 d_buf_size;
		bool strong_match;
		INT32 num_prefetches=0;
		t = AccessTagWLRU(ip, &found);
		IndexItem * entry = t->Item;
		
		if(found == false)
		{
			t->tag = ip;
			entry->index = ghb_buf->push(m_addr, -1);
			return 0;
		}
		else
		{
			if(entry->index < ghb_size)
			{ //accessing ghb
			
				INT32 last_index = entry->index;
				entry->index = ghb_buf->access_ghb(m_addr, last_index, d_buf, &d_buf_size);
		
				if((last_index != entry->index) && (ghb_buf->older(last_index,entry->index)))
				{
					// Local delta context
					num_prefetches = LocalDeltaContext(d_buf_size, d_buf, m_addr, pref_addr, &strong_match);
					*n_pref_DC = num_prefetches;
			
					if(num_prefetches > 0)
					{
						_Delta_Buffer *t;
						int t_ind;
						t_ind = d_bufs->get_LRU_buf();		  
						t = d_bufs->get_i(t_ind);
						t->copy(d_buf, LDB_SIZE);
						t->set_ip(ip);
						t->set_last_addr(m_addr);
						//update LRU of all d_bufs
						d_bufs->increase_LRU();
						t->set_LRU(0);
						t->set_last_c_stride(d_buf[0]);
						
						t->set_stride(false);
						if(num_prefetches == PREF_BUF_SIZE)
							if(((pref_addr[0] == m_addr + d_buf[0]) && (pref_addr[3] == m_addr + 4 * d_buf[0])&& (pref_addr[7] == m_addr + 8 * d_buf[0])) || strong_match)
						t->set_stride(true);
						
						entry->index = ghb_size + t_ind;
					}

					// Local delta context simple
					if(0 == num_prefetches)
					{    
						num_prefetches += LocalDeltaContextSimple(d_buf_size, d_buf, m_addr, pref_addr);
						*n_pref_DC_s = num_prefetches;
					}
					
					// Global stride
					if(PREF_BUF_SIZE > num_prefetches){
						*n_pref_GS = num_prefetches;
						num_prefetches += GlobalStride(entry->index, m_addr, pref_addr, num_prefetches);
						*n_pref_GS = num_prefetches - *n_pref_GS;
					}
				}

				//assert(PREF_BUF_SIZE >= num_prefetches);
				return num_prefetches;
			}
			else
			{
				//accessing delta buffers
				int d_index = entry->index - ghb_size;
		
				_Delta_Buffer * td = d_bufs->get_i(d_index);
				if(ip == td->get_ip())
				{
					INT32 c_stride = m_addr - td->get_last_addr();
					td->push(c_stride);
					td->set_last_addr(m_addr);
					d_bufs->update_LRU(d_index);
					num_prefetches = LocalDeltaContext(td->get_size(), td->get_delta(), m_addr, pref_addr, &strong_match);
				
				if(num_prefetches == PREF_BUF_SIZE) 
				{
					if(((pref_addr[0] == m_addr + c_stride) && pref_addr[3] == m_addr + 4 * c_stride && pref_addr[7] == m_addr + 8 * c_stride) || strong_match)
					{
						if(td->get_stride() == true)
						{
							pref_addr[0] = pref_addr[PREF_BUF_SIZE - 1];
							num_prefetches = 1;
						}
					else
						td->set_stride(true);
				}
				else
				{
					td->set_stride(false);
				}
			}
			else
			{
				td->set_stride(false);
			}
			
			if(0 == num_prefetches)
				num_prefetches += LocalDeltaContextSimple(td->get_size(), td->get_delta(), m_addr, pref_addr);
			
			*n_pref_DC = num_prefetches;
			num_prefetches_from_delta += num_prefetches;
			
			if(0 == num_prefetches)
			{
				num_prefetches++;
				pref_addr[0] = m_addr + td->get_last_c_stride();
				pref_addr[1] = m_addr + 64; //next line prefetch
				num_prefetches++;
			}
			else
			{
				td->set_last_c_stride(c_stride);
			}
			return num_prefetches;
		}
		else
		{
			t->tag = ip;
			entry->index = ghb_buf->push(m_addr, -1);
			//assert(entry->index >= 0 && entry->index < ghb_size);
			return 0;
		}
	}
}

	bool Hit(UINT32 threadId, ADDRINT ip, ADDRINT memAddr )
	{
	Tag<IndexItem> * t;
	bool found;
	t = AccessTag(ip, &found);
	if(found == false) 
		return false;
	else
	{
		//assert(t != NULL);
		//assert(t->Item->index >= 0 && t->Item->index < ghb_size);
		INT32 delta = ghb_buf->ghb[t->Item->index].mem_addr - memAddr;
		if(delta < 64 && delta > -64) return false;  
		return true;
	} 
	}
	void dump()
	{
	cout << "index table:" << endl;
	for(int i = 0; i < num_lines; i++)
		for(int j = 0; j < num_sets; j++)
		{
	cout << "Tag :" << hex << TagArray[i][j].tag << dec << " index: " << TagArray[i][j].Item->index << " lru: "<< TagArray[i][j].LRU << endl;
		}
	cout << "GHB:" << endl;
	ghb_buf->dump();
	}
};

class MSHR : _CacheLikeTable<ADDRINT>
{
 public:
 MSHR(UINT32 tableSize, UINT32 set_asso)
	: _CacheLikeTable<ADDRINT>(tableSize/set_asso, set_asso) {}//(1, tableSize){}
	~MSHR(){};
	
	// Look-up in the mshr table. This leads to a subsequent LRU update in the table.
	// Returns 1 if hit and 0 if miss.
	bool AccessEntry(ADDRINT memAddr)
	{
	Tag<ADDRINT> * t;
	bool found;
	t = AccessTagWLRU(memAddr, &found);
	if(found == false)
		t->tag = memAddr;
	return(found);
	}

	bool Hit(ADDRINT memAddr){
	bool found; 
	Tag<ADDRINT> * t;
	t = AccessTag(memAddr, &found);
	return(found);
	}

	// Scan the MSHR and test the prefetch bit for each address. If the prefetch bit
	// is not -1, that means that the addr is already in the cache and we can 
	// release the MSHR entry. Set the address of the released entry to invalid
	// and move it to the tail of the queue (LRU). 
	void Release(COUNTER cycle, UINT32 id)
	{
	INT32 pref_bit;
	for(int k = 0; k < num_lines; k++)
	{
		for(int j = 0; j < num_sets; j++)
		{
	pref_bit = GetPrefetchBit(id, (TagArray[k][j].tag << 6));
	if(pref_bit != -1)
		{
#ifdef GEN_TRACE
	 cout << "MSHR" << id << " cycle: " << cycle << " releasing addr: " << hex << TagArray[k][j].tag << dec << endl;
#endif	
	 TagArray[k][j].tag = INVALID_ADDR;
	 for(int i = 0; i < num_sets; i++)
	 {
		 if(TagArray[k][i].LRU > TagArray[k][j].LRU)
		 TagArray[k][i].LRU--;
	 }
	 TagArray[k][j].LRU = num_sets - 1;	 
	}
		}
	}
	}

};
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
		 for(INT32 i=0; i<num_entries; i++)
	 array[i] =0;
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
class SamplePrefetcher : public GHBPrefetcher
{
	private:
	
	// Statistics
	UINT32 num_prefetches; // number of issued prefetches
	UINT32 num_partial_prefetches; // number of issued prefetches
	UINT32 num_good_prefetches; // number of prefetches followed by a hit
	
 public:
	
 SamplePrefetcher() : GHBPrefetcher(256, 8, 128) //256 entry 8-way Index table, 128-entry GHB 
 {
	 num_prefetches=0;
	 num_partial_prefetches=0;
	 num_good_prefetches=0;
 }
	
	void inc_num_prefetches(){
	num_prefetches++;
	}
	
	void inc_num_good_prefetches(){
	num_good_prefetches++;
	}

	void inc_num_partial_prefetches(){
	num_partial_prefetches++;
	}	  
	
	void PrintStats(){
	GHBPrefetcher::PrintStats();
	cout << "num_prefetches: " << num_prefetches;
	cout << " num_partial_prefetches: " << num_partial_prefetches;
	cout << " num_good_prefetches: " << num_good_prefetches << endl;
	}

	void reset_stats() {num_prefetches = 0; num_partial_prefetches = 0; num_good_prefetches = 0;}
	
	UINT32 get_num_prefetches() { return num_prefetches;}
	UINT32 get_num_good_prefetches () {return num_good_prefetches;}
	UINT32 get_num_partial_prefetches() {return num_partial_prefetches;} 
	
	~SamplePrefetcher(){  }
};


#endif
