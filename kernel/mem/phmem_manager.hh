#pragma once

namespace mem {
class phmem_manager {
public:
	virtual bool back_vmem(x64::linaddr where, uint64_t size, uint8_t flags) = 0;
	virtual void unback_vmem(x64::linaddr where, uint64_t size, uint8_t flags) = 0;
}

x64::phaddr resolve_to_phmem(x64::linaddr to_resolve); // -1 if linaddr is not mapped
void switch_paging(x64::linaddr highlevel_table);


/* The phmem_small_manager is a *static* physical memory manager.
* It handles a fixed size of 1 GiB of memory. It is needed as the kernel's
* (first) memory manager because other MMs might use dynamic memory themselves (for instance to
* add page directories etc.)
*/

template<typename Allocator>
struct phmem_manager : phmem_manager {
public:
	phmem_manager(uint64_t sz);
	bool back_vmem(x64::linaddr where, uint64_t size, uint8_t flags) override;
	void unback_vmem(x64::linaddr where, uint64_t size, uint8_t flags) override;

	void reserve(x64::phyaddr start, uint64_t size); // Register physical memory as unusable for allocation
private:
	template<int lv> 
	bool crawl_tables(x64::linaddr start, x64::linaddr end, uint16_t flags, hl_page_table* table) {

	uint64_t mem_size;
	Allocator alloc;
}

}
