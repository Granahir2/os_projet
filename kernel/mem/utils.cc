#include "utils.hh"
#include "x86/asm_stubs.hh"
#include "x86/memory.hh"
using namespace x64;
#include "kernel/kernel.hh"
namespace mem {

phmem_resolvant resolve_to_phmem(linaddr to_resolve) {
	bool isrw = true;

	auto lvl4 = (pml4*) (get_cr3() - 512*1024*1024*1024ul); // We assume identity mapping of physical memory
	if((int64_t)(to_resolve) <= -(1ll << 48) || (int64_t)(to_resolve) >= (1ll << 48)) {
		return {phmem_resolvant::NOT_CANONICAL, isrw, (phaddr)(-1)};
	}
	to_resolve &= (1ull << 48) - 1;

	hl_paging_entry val = lvl4->entry[(to_resolve >> 39) & 0x1ff];
	isrw &= (val.content & hl_paging_entry::RW) != 0;
	if(!(val.content & hl_paging_entry::P)) {
		puts("Fail at lvl4");
		return {phmem_resolvant::NOT_MAPPED, isrw, (phaddr)(-1)};
	}

	constexpr uint64_t address_mask = ~(0xfff0000000000fff);

	auto lvl3 = (pml3*)((val.content & address_mask) - 512*1024*1024*1024ul);
	val = lvl3->entry[(to_resolve >> 30) & 0x1ff];
	isrw &= (val.content & hl_paging_entry::RW) != 0;
	if(!(val.content & hl_paging_entry::P)) {
		puts("Fail at lvl3");
		return {phmem_resolvant::NOT_MAPPED, isrw, (phaddr)(-1)};
	}

	auto lvl2 = (page_directory*)((val.content & address_mask) - 512*1024*1024*1024ul);
	val = lvl2->entry[(to_resolve >> 21) & 0x1ff];
	isrw &= (val.content & hl_paging_entry::RW) != 0;
	if(!(val.content & hl_paging_entry::P)) {
		puts("Fail at lvl2");
		return {phmem_resolvant::NOT_MAPPED, isrw, (phaddr)(-1)};
	}
	if(val.content & hl_paging_entry::PS) {
		return {phmem_resolvant::OK, isrw,(val.content & address_mask) | (to_resolve & 0x1fffff)};
	}

	auto pg_table = (page_table*)((val.content & address_mask) - 512*1024*1024*1024ul);
	auto pt_entry = pg_table->entry[(to_resolve >> 12) & 0x1ff];
	isrw &= (pt_entry.content & pte::RW) != 0;
	if(!(pt_entry.content & pte::P)) {
		puts("Fail at pagelevel");
		return {phmem_resolvant::NOT_MAPPED, isrw, (phaddr)(-1)};
	}

	return {phmem_resolvant::OK, isrw, (pt_entry.content & address_mask) | (to_resolve & 0xfff)};
}

}
