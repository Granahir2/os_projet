#pragma once
#include "x86/memory.hh"
#include <cstddef>

void kprintf(const char* str, ...);
extern "C" void halt();
extern "C" void* kmmap(void* hint, size_t sz);
x64::phaddr get_phpage();
void release_phpage(x64::phaddr);
