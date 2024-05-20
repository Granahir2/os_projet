#pragma once

#include "mem/phmem_allocator.hh"
#include "mem/phmem_manager.hh"
#include "mem/vmem_allocator.hh"
#include "x86/memory.hh"
#include <cstddef>
#include "misc/memory_map.hh"
#include "kernel/interrupts.hh"
#include "x86/descriptors.hh"

// Linker defined symbols
extern "C" int kernel_begin;
extern "C" int kernel_end;

extern "C" void abort [[noreturn]] ();
extern "C" void halt [[noreturn]] ();
extern "C" void assert(bool, const char*);
extern "C" void* kmmap(void* hint, size_t sz);
extern "C" void kmunmap(void* ptr, size_t sz);
x64::phaddr get_phpage();
void release_phpage(x64::phaddr);

void setup_heap(memory_map_entry* mmap, uint64_t kernel_zero);

extern mem::ph_tree_allocator<10>* palloc;
extern mem::VirtualMemoryAllocator<18>* pkvmem_alloc;
extern mem::phmem_manager* pphmem_man;


struct __attribute__((packed)) _gdt {
	x64::segment_descriptor null_entry;
	x64::segment_descriptor code_segment; // 0x08
	x64::segment_descriptor data_segment; // 0x10
	x64::segment_descriptor code_segment_ring3; // 0x18
	x64::segment_descriptor data_segment_ring3; // 0x20
	x64::system_segment_descriptor tss_desc; // 0x28...
};
extern _gdt* pgdt;
extern x64::TSS* ptss;
extern interrupt_manager* pimngr;
