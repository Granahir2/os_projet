#pragma once
#include <cstddef>

long int strtol(const char* nptr, const char** endptr, int base);

extern "C" {
void* malloc(size_t size);
void  free(void* ptr);
}
