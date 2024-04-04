#pragma once
#include <cstddef>
#include <cstdint>

void* operator new(size_t len);
void* operator new[](size_t len);

void  operator delete(void* ptr, size_t len);
void  operator delete[](void* ptr);
