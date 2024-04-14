#pragma once
#include "kernel/fs/fs.hh"

extern "C" {

typedef filehandler FILE; // Probably temporary
extern FILE* stdout;
constexpr static int EOF = -1;

int puts(const char* str);
int printf(const char* src,...);

//int sscanf(const char* src, const char* format,...);

size_t fwrite(const void* buffer, size_t size, size_t cnt, FILE* f);


}
