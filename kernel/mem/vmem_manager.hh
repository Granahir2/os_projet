#pragma once
//#include "vmem_small_manager.hh"
#include "x86/memory.hh"

namespace mem {
class vmem_manager {
	virtual x64::linaddr valloc(x64::linaddr prefer, uint32_t size) = 0;
	virtual void vunalloc(x64::linaddr ptr, uint32_t size) = 0;
};

class vmem_small_manager : vmem_manager {};

template<int N>
class vmem_fixed_manager : vmem_manager {
public:
	vmem_fixed_manager(x64::linaddr offset) : off(offset) {
		for(x64::linaddr i = 0; i < N; ++i)
			{smallarr[i] = vmem_small_manager(offset + 1024*1024*1024*i);}
	};
	~vmem_fixed_manager() = default;

	x64::linaddr valloc(x64::linaddr prefer, uint32_t size) override {
		x64::linaddr retval = -1;
		if(size > 1024*1024*1024) {return retval;}
		for(int i = 0; i < N && (retval = smallarr[N].valloc(prefer, size)) == -1; ++i);
		return retval;
	}

	void vunalloc(x64::linaddr ptr, uint32_t size) override {
		auto k = (ptr - off) / (1024*1024*1024);
		auto kp = (ptr + size - 1 - off) / (1024*1024*1024);
	
		if(k < 0 || kp >= N) {return;} // Calling on invalid ranges is UB

		if(k == kp) {
			smallarr[k].vunalloc(ptr, size);
		} else {
			auto sz = size;
			smallarr[k].vunalloc(ptr, off + (k+1)*1024*1024*1024 - ptr);
			sz -= off + (k+1)*1024*1024*1024 - ptr;
			for(auto i = k + 1; i < kp && i < N; ++i) {
				smallarr[i].vunalloc(ptr, 1024*1024*1024);
				sz -= 1024*1024*1024;
			}
			smallarr[kp].vunalloc(ptr, sz);
		}
	}

private:	
	vmem_small_manager smallarr[N];
	x64::linaddr off;
};



}
