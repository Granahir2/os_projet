#include "phmem_manager.hh"
#include "utils.hh"
#include "x86/asm_stubs.hh"
#include "kernel/kernel.hh"
#include "kstdlib/cstring.hh"
#include "kstdlib/cstdio.hh"

#include <cstddef>

namespace mem {


x64::page_table phmem_manager::kpt_repo[1024]; // When the kernel needs *page tables* it will fetch it here, spliting the PDs with PS=1.
int phmem_manager::offset;

using namespace x64;

constexpr uint64_t address_mask = ~0xfff0000000000fff;

constexpr uint64_t to_the_end = 0xffffffffffffffff;
constexpr uint64_t from_the_beginning = 0;

template<typename V>
void crawl_tables_lvl1(linaddr start, linaddr end, hl_page_table* table, V& callback_page) {
	page_table* pt = reinterpret_cast<page_table*>(table);
	auto start_index = (start >> 12) & 0x1ff;
	auto end_index   = (end >> 12) & 0x1ff;

	callback_page(&pt->entry[start_index]);
	if(start_index == end_index) {return;};
	for(unsigned i = start_index+1; i <= end_index;++i) {
		callback_page(&pt->entry[i]);
	}
}


template<int lv, typename U, typename V>
void crawl_tables(linaddr start, linaddr end, hl_page_table* table, U& callback_dir, V& callback_page,
bool iskernel=false) {
if constexpr (lv == 1) {
	crawl_tables_lvl1(start, end, table, callback_page);
} else {
	uint16_t start_index = (start >> (12+(lv-1)*9)) & 0x1ff;
	uint16_t end_index = (end >> (12 + (lv-1)*9)) & 0x1ff;

	if(!(table->entry[start_index].content & hl_paging_entry::OS_CRAWLABLE)) {
		hl_page_table* newtable;
		if (lv == 2 && iskernel && phmem_manager::offset < 1024) {
			newtable = (hl_page_table*)(&phmem_manager::kpt_repo[phmem_manager::offset++]);
		} else {
			newtable = (hl_page_table*)kmmap(NULL, 4096);
			memset(newtable, 0, sizeof(hl_page_table));
		}
		table->entry[start_index].content = resolve_to_phmem((x64::linaddr)(newtable)).resolved | (table->entry[start_index].content & 0xfff);
		table->entry[start_index].content |= hl_paging_entry::OS_CRAWLABLE;
		table->entry[start_index].content &= ~hl_paging_entry::PS;
	}

	callback_dir(&table->entry[start_index]);
	if(start_index == end_index) {
		crawl_tables<lv-1>(start, end, (hl_page_table*)((table->entry[start_index].content & address_mask) - 512*1024*1024*1024ul), callback_dir, callback_page, iskernel);
		return;
	} else {
		crawl_tables<lv-1>(start, to_the_end, (hl_page_table*)((table->entry[start_index].content & address_mask) - 512*1024*1024*1024ul), callback_dir, callback_page, iskernel);
	}

	for(int i = start_index + 1; i < end_index; ++i) {
		if(!(table->entry[i].content & hl_paging_entry::OS_CRAWLABLE)) {
			hl_page_table* newtable;
			if (lv == 2 && iskernel && phmem_manager::offset < 1024) {
				newtable = (hl_page_table*)(&phmem_manager::kpt_repo[phmem_manager::offset++]);
			} else {
				newtable = (hl_page_table*)kmmap(NULL, 4096);
				memset(newtable, 0, sizeof(hl_page_table));
			}
			table->entry[i].content = resolve_to_phmem((x64::linaddr)(newtable)).resolved | (table->entry[end_index].content & 0xffff);
			table->entry[i].content |= hl_paging_entry::OS_CRAWLABLE;
			table->entry[i].content &= ~hl_paging_entry::PS;
		}
		callback_dir(&table->entry[i]); 
		crawl_tables<lv-1>(from_the_beginning, to_the_end, (hl_page_table*)((table->entry[i].content & address_mask) - 512*1024*1024*1024ul), callback_dir, callback_page, iskernel);
	}

	if(!(table->entry[end_index].content & hl_paging_entry::OS_CRAWLABLE)) {
		hl_page_table* newtable;
		if (lv == 2 && iskernel && phmem_manager::offset < 1024) {
			newtable = (hl_page_table*)(&phmem_manager::kpt_repo[phmem_manager::offset++]);
		} else {
			newtable = (hl_page_table*)kmmap(NULL, 4096);
			memset(newtable, 0, sizeof(hl_page_table));
		}
		table->entry[end_index].content = resolve_to_phmem((x64::linaddr)(newtable)).resolved | (table->entry[end_index].content & 0xffff);
		table->entry[end_index].content |= hl_paging_entry::OS_CRAWLABLE;
		table->entry[end_index].content &= ~hl_paging_entry::PS;
	}

	callback_dir(&table->entry[end_index]);
	crawl_tables<lv-1>(from_the_beginning, end, (hl_page_table*)((table->entry[end_index].content & address_mask) - 512*1024*1024*1024ul), callback_dir, callback_page,iskernel);
}
}



phmem_manager::phmem_manager(uint64_t sz) : mem_size(sz) {}

bool phmem_manager::back_vmem(x64::linaddr where, uint64_t size, uint32_t flags, x64::phaddr eff_cr3) {	
	auto start = where;
	auto end = where - 1 + size;
	flags &= 0xfff;

	if((int64_t)where <= -(1ll << 48) || (int64_t)(where) >= (1ll << 48)) {
		return false;
	}
	bool iskernel = ((int64_t)where < 0) && ((int64_t)where >= -2*1024*1024*1024);
	// Crawl through the page tables, mapping pages
	// We *assume* physical memory is id-mapped.
	
	// PT (512 pages, 2 MiB in total)
	// PD (512 PT, 1 GiB in total)
	// PML3 (512 PD, 512 GiB in total)
	// PML4 (512 PML3)

	struct {
		void operator()(hl_paging_entry* hl_entry) {hl_entry->content |=  0b11 | f;};
		uint32_t f;
	} callback_dir;
	callback_dir.f = flags;
	struct {
		void operator()(pte* pt_entry) {pt_entry->content = (get_phpage() & address_mask) | 0b11 | f;};
		uint32_t f;
	} callback_page;
	callback_page.f = flags;

	auto cr3 = eff_cr3 ? (hl_page_table*)(eff_cr3 - 512*1024*1024*1024ul)
		: (hl_page_table*)(x64::get_cr3() - 512*1024*1024*1024ul);
	crawl_tables<4>(start,end,cr3,callback_dir,callback_page,iskernel);
	return true;
}

void phmem_manager::unback_vmem(x64::linaddr where, uint64_t size, [[maybe_unused]] uint32_t flags) {	
	auto start = where;
	auto end = where - 1 + size;
	if((int64_t)where <= -(1ll << 48) || (int64_t)(where) >= (1ll << 48)) {return;}
	bool iskernel = ((int64_t)where < 0) && ((int64_t)where >= -2*1024*1024*1024);
	struct {
		void operator()(hl_paging_entry*) {};
	} callback_dir;
	struct {
		void operator()(pte* pt_entry) {
			pt_entry->content &= ~0x1;
			release_phpage(pt_entry->content & address_mask);};
	} callback_page;

	auto cr3 = (hl_page_table*)(x64::get_cr3() - 512*1024*1024*1024ul);
	crawl_tables<4>(start,end,cr3,callback_dir,callback_page,iskernel);
}

}
