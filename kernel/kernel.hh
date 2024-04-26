#pragma once
#include "x86/memory.hh"
#include <cstddef>
#include "misc/memory_map.hh"

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

void setup_heap(memory_map_entry* mmap);
