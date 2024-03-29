#include "x86/memory.hh"
#include "kernel/kernel.hh"
namespace mem {
constexpr x64::linaddr alloc_null = (x64::linaddr)(-1);

template<typename T>
T max(T a, T b) {if(a > b) {return a;} else {return b;}}

template <short depth, short log2_page_size = 12>
class BuddyAlloc {
public: // maxdepth is the maximum depth/order of a free block under us
	BuddyAlloc() : maxdepth(depth) {}
	x64::linaddr get_range(size_t size) {
		if(maxdepth == -1 || (size > 1ull << (maxdepth + log2_page_size))) {return alloc_null;}

		if(size > mid_point) { // size > max_size/2 *but* we know we can fit it
			//maxdepth = -1; // Block is full !
			return 0;
		}

		// We need to fit the alloc into one of our children	
		if(maxdepth == depth) { // Propagate the free condition to our children if need be
			children[0].maxdepth = depth - 1;
			children[1].maxdepth = depth - 1;
		}

		x64::linaddr return_value = children[0].get_range(size);
		if(return_value == alloc_null) {
			return_value = mid_point + children[1].get_range(size);
		}
		//maxdepth = max(children[0].maxdepth, children[1].maxdepth);
		return return_value;
	}

	void free_range(x64::linaddr addr_begin, size_t size) {
		if(addr_begin >= total_size || size == 0) {return;}
		if(addr_begin == 0 && size >= total_size) {maxdepth = depth - 1; return;}

		if(maxdepth == -1) { // Propagate the fully occupied condition if need be
			children[0].maxdepth = -1;
			children[1].maxdepth = -1;
		}
		if(addr_begin >= mid_point) {
			children[1].free_range(addr_begin - mid_point, size);

			maxdepth = max(children[0].maxdepth, children[1].maxdepth);
			if(children[0].maxdepth == depth - 1 && children[1].maxdepth == depth - 1) {
				maxdepth = depth;
			}
			return;
		}
		if(addr_begin + size <= mid_point) {
			children[0].free_range(addr_begin, size);
			
			maxdepth = max(children[0].maxdepth, children[1].maxdepth);
			if(children[0].maxdepth == depth - 1 && children[1].maxdepth == depth - 1) {
				maxdepth = depth;
			}
			return;
		}
		
		children[0].free_range(addr_begin, (total_size/2) - addr_begin);
		children[1].free_range(0, size - (total_size/2) + addr_begin);
		
		maxdepth = max(children[0].maxdepth, children[1].maxdepth);
		if(children[0].maxdepth == depth - 1 && children[1].maxdepth == depth - 1) {
			maxdepth = depth;
		}
	}

	bool is_range_free(x64::linaddr addr_begin, size_t size) const {
		if(size == 0) {return true;}
		if(maxdepth == -1 || addr_begin + size >= total_size) {return false;}
		if(maxdepth == depth) {return true;}

		// We know we are neither free or full : the status of our children is correct
		if(addr_begin >= mid_point) {
			return children[1].is_range_free(addr_begin - mid_point, size);
		}
		if(addr_begin + size <= mid_point) {
			return children[0].is_range_free(addr_begin, size);
		}

		return children[0].is_range_free(addr_begin, (total_size/2) - addr_begin) &&
		children[1].is_range_free(0, size - (total_size/2) + addr_begin);
	}

	// Seems like we have O(Nln N), but in fact we have O(ln N)
	// (consider the special case where one bound is 0 or total_size - 1)
	void mark_used(x64::linaddr addr_begin, size_t size) {
		if(maxdepth == -1 || size == 0) {return;}
		if(addr_begin == 0 && size >= total_size) {maxdepth = -1; return;};

		// If fully free, we need to propagate
		if(maxdepth == depth) {children[0].maxdepth = depth - 1; children[1].maxdepth = depth - 1;}
		
		if(addr_begin >= mid_point) {
			children[1].mark_used(addr_begin - mid_point, size);
			maxdepth = max(children[0].maxdepth, children[1].maxdepth);
			return;
		}
		if(addr_begin + size <= mid_point) {
			children[0].mark_used(addr_begin, size);
			maxdepth = max(children[0].maxdepth, children[1].maxdepth);
			return;
		}

		children[0].mark_used(addr_begin, (total_size/2) - addr_begin);
		children[1].mark_used(0, size - (total_size/2) + addr_begin);
		maxdepth = max(children[0].maxdepth, children[1].maxdepth);
	}

	char maxdepth;
	BuddyAlloc<depth-1, log2_page_size> children[2];
	static constexpr x64::linaddr mid_point = 1 << (depth - 1 + log2_page_size);
	static constexpr x64::linaddr total_size = 1 << (depth + log2_page_size);
};

template <short log2_page_size>
class BuddyAlloc<0, log2_page_size> {
public:
	char maxdepth;
	static constexpr x64::linaddr total_size = 1 << log2_page_size;
	BuddyAlloc() : maxdepth(0) {}
	x64::linaddr get_range(size_t size) {
        	if(size > total_size || maxdepth == -1) {return alloc_null;}
		else {maxdepth = -1; return 0;}
    	}
	bool is_range_free(x64::linaddr addr, size_t size) const {
		return (addr + size <= total_size) && (maxdepth == 0);
	}
	void free_range(x64::linaddr addr, size_t size) {
		if(addr < total_size && size != 0) {maxdepth = 0;}
	}
	void mark_used(x64::linaddr addr_begin, size_t size){
		if(size > 0 && addr_begin < total_size) {maxdepth = -1;} 
	}
};


/*
Beware : we may over-allocate and then munmap leaves garbage; this has to be
handled outside the allocator. Probably we use a table of entries like
base/apparent len/effective len
*/
template <short depth, short log2_page_size = 12>
class VirtualMemoryAllocator 
{
public:
    x64::linaddr mmap(x64::linaddr addr, size_t size) {
	if(size > (1ull << (depth + log2_page_size)) || size == 0) {return alloc_null;}
	if(root.is_range_free(addr, size)) {
		root.mark_used(addr, size);
		return addr;
	} else {
		auto retval = root.get_range(size);
		root.mark_used(retval,size);
		return retval;
	}
    }
    void munmap(x64::linaddr addr, size_t size)
    {
        root.free_range(addr, size);
    }
    void mark_used(x64::linaddr addr_begin, size_t sz) {
	root.mark_used(addr_begin, sz);
    }
private:
    BuddyAlloc<depth> root;
};
}
