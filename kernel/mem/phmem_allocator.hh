#pragma once
#include "kstdlib/cstdio.hh"
#include "x86/memory.hh"
#include "kernel/kernel.hh"
namespace mem {

constexpr static x64::phaddr phalloc_null = (x64::phaddr)(-1);

template<int depth>
class ph_tree_allocator {
public:
	bool isfull() {return !free_cache;}
	// Allocations can only be page-sized.
	x64::phaddr get_page() {
		if(!free_cache) {
			return phalloc_null;
		} else {
			x64::phaddr x;
			int i = 0;
			for(;(i < 4) && (x = children[i].get_page()) == phalloc_null; ++i);

			free_cache = false;
			for(int j = 0; j < 4; ++j) {free_cache |= !children[j].isfull();};			

			return x + i*child_size;
		}
	}

	void release_page(x64::phaddr p) {
		auto k = p / child_size;
		if(k < 4) {
			children[k].release_page(p - k*child_size);
			if(!children[k].isfull()) {free_cache = true;}
		}
	}

	void mark_used(x64::phaddr p_beg, x64::phaddr p_end) {
		auto k_beg = p_beg / child_size;
		auto k_end = p_end / child_size;
		if(k_beg >= 4) return;
		if(k_end >= 4) k_end = 3;

		if(k_beg == k_end) {
			children[k_beg].mark_used(p_beg - k_beg*child_size,
						  p_end - k_beg*child_size); 
			free_cache = false;
			for(int j = 0; j < 4; ++j) {free_cache |= !children[j].isfull();};			
		} else {
			children[k_beg].mark_used(p_beg - k_beg*child_size, child_size - 1);
			children[k_end].mark_used(0, p_end - k_end*child_size);
			for(unsigned int i = k_beg + 1; i < k_end; ++i){
				children[i].mark_used(0, child_size - 1);
			}
	
			free_cache = false;
			for(int j = 0; j < 4; ++j) {free_cache |= !children[j].isfull();};			
		}
	}

	bool free_cache = true;
	constexpr static size_t total_size = 1 << (depth*2 + 12); // depth of 0 == 1 page
	constexpr static size_t child_size = 1 << ((depth-1)*2 + 12);
	ph_tree_allocator<depth - 1> children[4];
};

template<>
struct ph_tree_allocator<3> { // 64 = 4^3 pages
public:
	bool isfull() {return x == 0xffffffffffffffffull;};
	x64::phaddr get_page() {
		if(isfull()) {return phalloc_null;}
		else {
			for(int j = 0; j < 64; ++j) {
				if((~x >> j) & 1ull) {x |= (1ull<<j); return j*4096;};
			}
		}
		return phalloc_null;
	}
	void release_page(x64::phaddr p) {x &= ~(1ull << (p/4096));}
	void mark_used(x64::phaddr p_beg, x64::phaddr p_end) {
		auto k_beg = p_beg/4096;
		auto k_end = p_end/4096;
		if(k_beg >= 64) return;
		if(k_end >= 64) k_end = 63;

		for(unsigned i = k_beg; i <= k_end; ++i) {
			x |= (1ull << i);
		}

	}

	uint64_t x = 0;
};

}
