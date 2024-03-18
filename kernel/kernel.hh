#pragma once
#include "x86/memory.hh"
#include <cstddef>

void kprintf(const char* str, ...);
extern "C" void* kmmap(void* hint, size_t sz);
extern "C" void memset(void* p, int val, uint64_t len);
x64::phaddr get_phpage();
