#pragma once
#include <cstdint>
#include <cstddef>

using FILE = uint8_t;
using DIR = uint8_t;

extern "C" FILE* stdout;
extern "C" FILE* stdin;

size_t fread(void* buffer, size_t sz, size_t nmemb, FILE* f);
size_t fwrite(const void* buffer, size_t sz, size_t nmemb, FILE* f);

DIR*  get_root();

enum drit_status {DIR_ENTRY, FILE_ENTRY, LINK_ENTRY, UNDERRUN, NP};
drit_status traverse(const char* name, DIR* d);
FILE* open(const char* name, int mode, DIR* it);


void canonical_path(DIR* d, char* buffer, size_t n);
