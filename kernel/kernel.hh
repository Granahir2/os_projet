#pragma once
#include "x86/memory.hh"
#include <cstddef>

extern "C" void halt();
extern "C" void assert(bool, const char*);
extern "C" void* kmmap(void* hint, size_t sz);
extern "C" void kmunmap(void* ptr, size_t sz);
x64::phaddr get_phpage();
void release_phpage(x64::phaddr);
