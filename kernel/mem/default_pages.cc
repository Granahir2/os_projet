#include "x86/memory.hh"
#include "x86/asm_stubs.hh"
#include "utils.hh"
#include "default_pages.hh"
using namespace x64;

namespace mem {
constexpr page_directory make_linear_large_page_directory(uint64_t offset) {
	page_directory pagedir;
	for(int i = 0; i < 512; ++i) {
		pagedir.entry[i].content = (offset + i*2*1024*1024) | 0b0000010011011;
	}
	return pagedir;
};

constexpr struct {
	page_directory dir0 = make_linear_large_page_directory(0);
	page_directory dir1 = make_linear_large_page_directory(0x40000000);
	page_directory dir2 = make_linear_large_page_directory(0x80000000);
	page_directory dir3 = make_linear_large_page_directory(0xC0000000);
} idmap_low_pds;


struct {
	page_directory dir0 = make_linear_large_page_directory(0);
	page_directory dir1 = make_linear_large_page_directory(0x40000000);
} higherhalf_pds;

pml3 lower_pml3;
pml3 highhalf_pml3;

pml4 toplevel;


void default_pages_init(uint64_t kernel_zero) {
	lower_pml3.entry[0].content = resolve_to_phmem((linaddr)(&idmap_low_pds.dir0)).resolved | 0b11 | hl_paging_entry::OS_CRAWLABLE;
	lower_pml3.entry[1].content = resolve_to_phmem((linaddr)(&idmap_low_pds.dir1)).resolved | 0b11 | hl_paging_entry::OS_CRAWLABLE;
	lower_pml3.entry[2].content = resolve_to_phmem((linaddr)(&idmap_low_pds.dir2)).resolved | 0b11 | hl_paging_entry::OS_CRAWLABLE;
	lower_pml3.entry[3].content = resolve_to_phmem((linaddr)(&idmap_low_pds.dir3)).resolved | 0b11 | hl_paging_entry::OS_CRAWLABLE;

	higherhalf_pds.dir0 = make_linear_large_page_directory(kernel_zero);
	higherhalf_pds.dir1 = make_linear_large_page_directory(kernel_zero + 0x40000000);
	
	highhalf_pml3.entry[510].content = resolve_to_phmem((linaddr)(&higherhalf_pds.dir0)).resolved | 0b11 | hl_paging_entry::OS_CRAWLABLE;  
	highhalf_pml3.entry[511].content = resolve_to_phmem((linaddr)(&higherhalf_pds.dir1)).resolved | 0b11 | hl_paging_entry::OS_CRAWLABLE;

	toplevel.entry[0].content = resolve_to_phmem((linaddr)(&lower_pml3)).resolved | 0b11 | hl_paging_entry::OS_CRAWLABLE;
	toplevel.entry[511].content = resolve_to_phmem((linaddr)(&highhalf_pml3)).resolved | 0b11| hl_paging_entry::OS_CRAWLABLE;

	load_cr3(resolve_to_phmem((linaddr)(&toplevel)).resolved);
}

}
