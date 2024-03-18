#pragma once
#include "x86/memory.hh"

namespace mem {
class phmem_manager_int {
public:
	virtual bool back_vmem(x64::linaddr where, uint64_t size, uint32_t flags) = 0;
	virtual void unback_vmem(x64::linaddr where, uint64_t size, uint32_t flags) = 0;
};


/* The phmem_small_manager is a *static* physical memory manager.
* It handles a fixed size of 1 GiB of memory. It is needed as the kernel's
* (first) memory manager because other MMs might use dynamic memory themselves (for instance to
* add page directories etc.)
*/

struct phmem_manager : phmem_manager_int {
public:
	phmem_manager(uint64_t sz);
	bool back_vmem(x64::linaddr where, uint64_t size, uint32_t flags) override;
	void unback_vmem(x64::linaddr where, uint64_t size, uint32_t flags) override;

	
	static x64::page_table kpt_repo[1024]; // When the kernel needs *page tables* it will fetch it here, spliting the PDs with PS=1.
	static int offset;

	//void reserve(x64::phyaddr start, uint64_t size); // Register physical memory as unusable for allocation
private:
	uint64_t mem_size;
};

}
