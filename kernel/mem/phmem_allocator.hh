#pragma once
#include "x86/memory.hh"
#include "kernel/kernel.hh"
namespace mem {

template<int depth>
class ph_tree_allocator {
public:
	bool isfull() {return !free_cache;}
	// Allocations can only be page-sized.
	x64::phaddr get_page() {
		if(!free_cache) { return -1;}
		else {
			x64::phaddr x;
			int i = 0;
			for(; (x = children[i].get_page()) == (x64::phaddr)(-1); ++i){}
			free_cache = false;
			for(int j = 0; j < 3; ++j) {free_cache |= !children[j].isfull();};			

			if(free_cache == false) {
				kprintf("Filled depth = %d\n", depth);
			}

			return x + i*(1ull << (12 + (depth-1)*2));
		}
	}

	void release_page(x64::phaddr p) {
		auto k = p >> (depth*2 + 12);
		if(k < 4) {
			children[k].release_page(p - k*(1ull << (12+(depth-1)*2) ));
			if(!children[k].isfull()) {free_cache = true;}
		}
	}

	void mark_used(x64::phaddr p_beg, x64::phaddr p_end) {
		auto k_beg = p_beg >> ((depth-1)*2 + 12);
		auto k_end = p_end >> ((depth-1)*2 + 12);

		if(k_beg == k_end) {
			children[k_beg].mark_used(p_beg - k_beg*(1ull << (12 + (depth-1)*2)),
						  p_end - k_beg*(1ull << (12 + (depth-1)*2))); 
			free_cache = false;
			for(int j = 0; j < 3; ++j) {free_cache |= !children[j].isfull();};			
		} else {
			children[k_beg].mark_used(p_beg - k_beg*(1ull << (12 + (depth-1)*2)), (1 << (12 + (depth-1)*2)) - 1);
			children[k_end].mark_used(0, p_end - k_end*(1ull << (12+(depth-1)*2)));
			for(unsigned int i = k_beg + 1; i < k_end; ++i){children[i].mark_used(0, (1ull << (12 + (depth-1)*2)) - 1);}
			for(int j = 0; j < 3; ++j) {free_cache |= !children[j].isfull();};			
		}
	}
private:
	bool free_cache = true;
	ph_tree_allocator<depth - 1> children[4];
};

template<>
struct ph_tree_allocator<3> {
public:
	bool isfull() {return x == 0xffffffffffffffff;};
	x64::phaddr get_page() {
		if(isfull()) {return -1;}
		else {
			for(int j = 0; j < 64; ++j) {
				if((~x >> j) & 1) {x |= (1<<j); return j*4096;};
			}
		}
		return -1;
	}
	void release_page(x64::phaddr p) {x &= ~(1 << (p/4096));}
	void mark_used(x64::phaddr p_beg, x64::phaddr p_end) {
		auto k_beg = p_beg / 4096;
		auto k_end = p_end / 4096;
		x |= ((1 << (k_end + 1)) - 1) ^ ((1 << (k_beg + 1)) - 1);
	}
private:
	uint64_t x = 0;
};

}
