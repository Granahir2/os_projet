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

		if(size > mid_point) { // size >= max_size/2 *but* we know we can fit it
			maxdepth = -1; // Block is full !
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
		maxdepth = max(children[0].maxdepth, children[1].maxdepth);
		return return_value;
	}

	void free_range(x64::linaddr addr, size_t size) {
		if(addr >= total_size || size == 0) {return;}
		if(size > mid_point) {maxdepth = depth - 1;}
		if(addr < mid_point && addr + size > mid_point) {maxdepth = depth - 1;}

		if(maxdepth == -1) { // Propagate the fully occupied condition if need be
			children[0].maxdepth = -1;
			children[1].maxdepth = -1;
		}

		if(addr < mid_point) {
			children[0].free_range(addr, size);
		} else {
			children[1].free_range(addr - mid_point, size);
		}

		maxdepth = max(children[0].maxdepth, children[1].maxdepth);
		if(children[0].maxdepth == depth - 1 && children[1].maxdepth == depth - 1) {
			maxdepth = depth;
		}
	}

	bool is_range_free(x64::linaddr addr, size_t size) const {
		if(size == 0) {return true;}
		if(maxdepth == -1 || addr + size > total_size) {return false;}
		if(maxdepth == depth) {return true;}
		if(size > mid_point) {return maxdepth == depth;} // that is, return false...
		if(addr < mid_point && addr + size > mid_point) {return maxdepth == depth;}

		// We know that the whole block isn't free or used, so the children are correctly tagged
		if(addr < mid_point) {return children[0].is_range_free(addr, size);}
		else {return children[1].is_range_free(addr - mid_point, size);}
	}
	void mark_used(x64::linaddr addr_begin, size_t size) {
		if(maxdepth == -1 || size == 0) {return;}
		if(size > mid_point) {maxdepth = -1; return;}
		if(size > mid_point ||
		   (addr_begin < mid_point && addr_begin + size > mid_point)){
		maxdepth = -1;
		return;
		}

		// If fully free, we need to propagate
		if(maxdepth == depth) {children[0].maxdepth = depth - 1; children[1].maxdepth = depth - 1;}

		if(addr_begin < mid_point) {children[0].mark_used(addr_begin, size);}
		else {children[1].mark_used(addr_begin - mid_point, size);}
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

template <short depth, short log2_page_size = 12>
class VirtualMemoryAllocator 
{
public:
    x64::linaddr mmap(x64::linaddr addr, size_t size) {
	if(root.is_range_free(addr, size)) {
		root.mark_used(addr, size);
		return addr;
	} else {
		return root.get_range(size);
	}
    }
    void munmap(x64::linaddr addr, size_t size)
    {
        root.free_range(addr, size);
    }
private:
    BuddyAlloc<depth> root;
};
}
