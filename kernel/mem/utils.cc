#include "utils.hh"
#include "x86/asm_stubs.hh"
#include "x86/memory.hh"
using namespace x64;
#include "kernel/kernel.hh"
namespace mem {

phmem_resolvant resolve_to_phmem(linaddr to_resolve) {
	auto lvl4 = (pml4*) (get_cr3()); // We assume identity mapping of physical memory
	if((int64_t)(to_resolve) <= -(1ll << 48) || (int64_t)(to_resolve) >= (1ll << 48)) {
		return {phmem_resolvant::NOT_CANONICAL, (phaddr)(-1)};
	}
	to_resolve &= (1ull << 48) - 1;

	hl_paging_entry val = lvl4->entry[(to_resolve >> 39) & 0x1ff];
	if(!(val.content & hl_paging_entry::P)) {
		return {phmem_resolvant::NOT_MAPPED, (phaddr)(-1)};
	}

	constexpr uint64_t address_mask = ~(0xfff0000000000fff);

	auto lvl3 = (pml3*)(val.content & address_mask);
	val = lvl3->entry[(to_resolve >> 30) & 0x1ff];
	if(!(val.content & hl_paging_entry::P)) {
		return {phmem_resolvant::NOT_MAPPED, (phaddr)(-1)};
	}

	auto lvl2 = (page_directory*)(val.content & address_mask);
	val = lvl2->entry[(to_resolve >> 21) & 0x1ff];
	if(!(val.content & hl_paging_entry::P)) {
		return {phmem_resolvant::NOT_MAPPED, (phaddr)(-1)};
	}
	if(val.content & hl_paging_entry::PS) {
		return {phmem_resolvant::OK, (val.content & address_mask) | (to_resolve & 0x1fffff)};
	}

	auto pg_table = (page_table*)(val.content & address_mask);
	auto pt_entry = pg_table->entry[(to_resolve >> 12) & 0x1ff];
	if(!(pt_entry.content & pte::P)) {
		return {phmem_resolvant::NOT_MAPPED, (phaddr)(-1)};
	}

	return {phmem_resolvant::OK, (pt_entry.content & address_mask) | (to_resolve & 0xfff)};
}

}
