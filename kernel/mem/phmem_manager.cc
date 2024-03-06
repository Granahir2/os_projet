#include "phmem_manager.hh"

namespace mem {

template<typename Allocator>
phmem_manager::phmem_small_manager() : alloc() {}

template<int lv>
bool phmem_manager::crawl_tables(x64::linaddr start, x64::linaddr end, x64::linaddr offset, uint16_t flags, hl_page_table* table) {
	uint64_t start_index = start >> (12 + (lv-1)*9);
	uint64_t end_index = end >> (12 + (lv-1)*9);

	if(*table.entry[start_index].content & x64::hl_paging_entry::OS_CRAWLABLE == 0) {
		hl_page_table* newtable = kmmap(4096);
		memset(newtable, 0, sizeof(hl_page_table));
		*table.entry[start_index].content = resolve_to_phmem((x64::linaddr)(newtable)) | (*table.entry[start_index].content & 0xffff);
		*table.entry[start_index].content |= x64::hl_paging_entry::OS_CRAWLABLE;
	}

	if(*table.entry[end_index].content & x64::hl_paging_entry::OS_CRAWLABLE == 0) {
		hl_page_table* newtable = kmmap(4096);
		memset(newtable, 0, sizeof(hl_page_table));
		*table.entry[end_index].content = resolve_to_phmem((x64::linaddr)(newtable)) | (*table.entry[end_index].content & 0xffff);
		*table.entry[end_index].content |= x64::hl_paging_entry::OS_CRAWLABLE;
	}
	*table.entry[start_index].content |= flags;

	if(start_index == end_index) {
		return crawl_tables(start - (start_index << (12 + (lv - 1)*9), end - (start_index << (12 + (lv - 1)*9)),
					offset + (start_index << (12 + (lv-1)*9)), flags, *table.entry[start_index].content & 0x7ffffffff000);
	}

	bool retval = true; // For now, partial allocations because of OoM trash the heap and are irrecoverable
	*table.entry[end_index].content |= flags;
	retval &= crawl_tables(start - (start_index << (12+(lv-1)*9)), (1<<(12+(lv-1)*9) - 1), offset + (start_index << (12+(lv-1)*9)),
			flags, *table.entry[start_index].content & 0x7ffffff000);
	if(!retval){return retval;}
	retval &= craw_tables(0, end - (end_index << (12+(lv-1)*9), offset + (end_index << (12+(lv-1)*9)), flags, *table.entry[end_index].content & 0x7ffffff000));
	if(!retval){return retval;}

	for(int64_t i = start_index + 1; i < end_index; ++i) {
		*table.entry[i].content |= flags;	
		retval &= crawl_tables(0, 1<<(12+(lv-1)*9) - 1, offset + (i << (12+(lv-1)*9)), flags, *table.entry[i].content & 0x7ffffff000); 
		if(!retval){return retval;}
	}
	return true;
}
template<>
bool phmem_manager::crawl_tables<1>(x64::linaddr start, x64::linaddr end, x64::linaddr, uint16_t flags, hl_page_table* table) {
	page_table* table = reinterpret_cast<page_table*>(table);
	uint64_t start_index = start >> (12+(lv-1)*9);
	uint64_t end_index = end >> (12+(lv-1)*9);

	for(int64_t i = start_index + 1; i < end_index; ++i) {
		auto x = alloc.get_page();
		if(x == NULL) {return false;}
		*table.entry[i].content |= alloc.get_page();
		*table.entry[i].content = flags;
	}
	return true;
}


template<typename Allocator>
bool phmem_manager::back_vmem(x64::linaddr where, uint64_t size, uint16_t flags) {
	x64::phyaddr block = alloc.allocate(size);
	if(block == (x64::phyaddr)(-1)) {return false;}
	
	auto start = where;
	auto end = where - 1 + size;
	
	// Crawl through the page tables, mapping pages
	// We *assume* paging structures are id-mapped.
	
	// PT (512 pages, 2 MiB in total)
	// PD (512 PT, 1 GiB in total)
	// PML3 (512 PD, 512 GiB in total)
	// PML4 (512 PML3)

	auto cr0 = (x64::pml4*)(read_cr0());
	return crawl_tables<4>(start,end,0,flags,cr0);
}

}
