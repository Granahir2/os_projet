#pragma once
#include "kernel/fs/fs.hh"

extern "C" {

typedef filehandler FILE; // Probably temporary
extern FILE* stdout;
constexpr static int EOF = -1;

int fputc(int c, FILE* stream);
int putchar(int c);

int puts(const char* str);
int fputs(const char* str, FILE* stream);

int printf(const char* src,...);
int fprintf(FILE* stream, const char* src,...);
//int sscanf(const char* src, const char* format,...);

size_t fwrite(const void* buffer, size_t size, size_t cnt, FILE* f);


}
