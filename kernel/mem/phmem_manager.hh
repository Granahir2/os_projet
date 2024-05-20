#pragma once
#include "x86/memory.hh"

namespace mem {
struct phmem_manager{
public:
	phmem_manager(uint64_t sz);
	bool back_vmem(x64::linaddr where, uint64_t size, uint32_t flags, x64::phaddr eff_cr3 = 0);
	void unback_vmem(x64::linaddr where, uint64_t size, uint32_t flags);

	
	static x64::page_table kpt_repo[1024]; // When the kernel needs *page tables* it will fetch it here, spliting the PDs with PS=1.
	static int offset;

	//void reserve(x64::phyaddr start, uint64_t size); // Register physical memory as unusable for allocation
private:
	uint64_t mem_size;
};

}
