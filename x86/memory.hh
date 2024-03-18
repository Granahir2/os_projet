#pragma once
#include <stdint.h>

namespace x64 {
using linaddr = uint64_t;
using phaddr = uint64_t;

struct pte {
	enum attr {P = 1ull,  RW = 2, US = 4,
		     PWT = 8,PCD = 16, A = 32, D = 64,
		     PAT = 128, G = 256, XD = (1ull << 63)};
	uint64_t content;
}; static_assert(sizeof(pte) == 8);

struct __attribute__((aligned(4096))) page_table {pte entry[512];};

struct hl_paging_entry {
	enum attr {P = 1ull,  RW = 2, US = 4,
		     PWT = 8,PCD = 16, A = 32, D = 64,
		     PS = 128, G = 256, OS_CRAWLABLE = 512, PAT = 4096, XD = (1ull << 63)};
	uint64_t content;
}; static_assert(sizeof(hl_paging_entry) == 8);

struct __attribute__((aligned(4096))) page_directory {hl_paging_entry entry[512];};
struct __attribute__((aligned(4096))) pml3 {hl_paging_entry entry[512];}; // 1 GiB huge page
struct __attribute__((aligned(4096))) pml4 {hl_paging_entry entry[512];}; // PS is reserved


struct __attribute__((aligned(4096))) hl_page_table {hl_paging_entry entry[512];}; // Generic type
}
