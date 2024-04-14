#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <cpuid.h>

#include "multiboot.hh"
#include "x86/descriptors.hh"
#include "x86/memory.hh"
#include "boot/load_kernel.hh"
#include "misc/display.h"
#include "misc/memory_map.hh"

constexpr multiboot::header craft_multiboot_header() {
	multiboot::header header;
	header.magic = multiboot::header_magic;
	header.wanted_flags = multiboot::header_flags::PAGE_ALIGN | multiboot::header_flags::MEMORY_INFO;
	header.checksum = -header.magic - header.wanted_flags;
	return header;
};
extern "C" __attribute__((section(".multiboot")))
constexpr multiboot::header multiboot_header = craft_multiboot_header();

extern "C" {
memory_map_entry memory_map_buffer[256];
}

/***************** GDT ******************/
constexpr x86::segment_descriptor craft_early_code_segment() {
	x86::segment_descriptor retval;
	retval.access = 0b10011011; // RX segment, accessible only to Ring 0 
	retval.set_limit(0x0fffff); // Cover the first 4 GB of RAM
	retval.flags = 0b1100; // 32-bit mode page-granularity
	return retval;
};

constexpr x86::segment_descriptor craft_early_data_segment() {
	x86::segment_descriptor retval;
	retval.access = 0b10010011; // RW segment, accessible only to Ring 0
	retval.set_limit(0x0fffff);
	retval.flags = 0b1100;
	return retval;
};

struct __attribute__((packed)) {
	x86::segment_descriptor null_entry = x86::segment_descriptor();
	x86::segment_descriptor code_segment = craft_early_code_segment();
	x86::segment_descriptor data_segment = craft_early_data_segment();
} early_gdt; static_assert(sizeof(early_gdt) == 3*sizeof(x86::segment_descriptor));


extern "C" {x64::gdt_descriptor early_gdt_desc = {sizeof(early_gdt) - 1, 0};}
extern "C" void ready_gdt_desc() {
	early_gdt_desc.size = sizeof(early_gdt) - 1;
	early_gdt_desc.base = (uint64_t)(&early_gdt);
}

/***************** 4 level paging ******************/
struct __attribute__((aligned(4096))) page_directory
{
	uint64_t pde[512] = {0};
}; static_assert(sizeof(page_directory) == 4096);

constexpr page_directory make_linear_large_page_directory(uint64_t offset) {
	page_directory pagedir;
	for(int i = 0; i < 512; ++i) {
		pagedir.pde[i] = (offset + i*2*1024*1024) | 0b0000010000011;
	}
	return pagedir;
};

constexpr struct {
	page_directory dir0 = make_linear_large_page_directory(0);
	page_directory dir1 = make_linear_large_page_directory(0x40000000);
	page_directory dir2 = make_linear_large_page_directory(0x80000000);
	page_directory dir3 = make_linear_large_page_directory(0xC0000000);
} page_directory_table;

constexpr struct {
	page_directory dir0 = make_linear_large_page_directory(0);
	page_directory dir1 = make_linear_large_page_directory(0x40000000);
} page_directory_table_highhalf;

struct __attribute__((aligned(4096))) {
	uint64_t entry[512];
} level3_page_table;

struct __attribute__((aligned(4096))) {
	uint64_t entry[512];
} level3_page_table_highhalf;

extern "C" {

struct __attribute__((aligned(4096))) {
	uint64_t phymap_entry;
	uint64_t filler[510];
	uint64_t higherhalf_entry;
} level4_page_table;
}

void ready_paging() {
	level3_page_table.entry[0] = (uint64_t)(&page_directory_table.dir0) | 0b11 | x64::hl_paging_entry::OS_CRAWLABLE;
	level3_page_table.entry[1] = (uint64_t)(&page_directory_table.dir1) | 0b11 | x64::hl_paging_entry::OS_CRAWLABLE;
	level3_page_table.entry[2] = (uint64_t)(&page_directory_table.dir2) | 0b11 | x64::hl_paging_entry::OS_CRAWLABLE;
	level3_page_table.entry[3] = (uint64_t)(&page_directory_table.dir3) | 0b11 | x64::hl_paging_entry::OS_CRAWLABLE;
	
	level3_page_table_highhalf.entry[510] = (uint64_t)(&page_directory_table_highhalf.dir0) | 0b11 | x64::hl_paging_entry::OS_CRAWLABLE;  
	level3_page_table_highhalf.entry[511] = (uint64_t)(&page_directory_table_highhalf.dir1) | 0b11 | x64::hl_paging_entry::OS_CRAWLABLE;

	level4_page_table.phymap_entry = (uint64_t)(&level3_page_table) | 0b11| x64::hl_paging_entry::OS_CRAWLABLE;
	level4_page_table.higherhalf_entry = (uint64_t)(&level3_page_table_highhalf) | 0b11| x64::hl_paging_entry::OS_CRAWLABLE;
}

int strcmp(char* a, char* b) {
	auto c = a;
	auto d = b;
	for(;*c != *d && *c != '\0' && *d != '\0'; c++,d++) {}
	return *d - *c;
}

extern "C" void halt();
extern "C" const char bootstrapper_end; // linker defined symbol

extern "C" uint32_t bootstrap(uint32_t magic, multiboot::info* info) {
	Display terminal = Display();
	terminal.clear();
	terminal.print("Hello World ! 64bit preloader build " __TIMESTAMP__ "\n");
	if(magic == multiboot::bootloader_magic) {
		terminal.print("Multiboot confirmed.\n");
	} else { halt();}

	if(info->mmap_length <= 0 || info->mmap_length > 256) {
		terminal.print("Memory map not available or corrupted. Fatal.\n");
	} else {
		auto* ptr = (char*)info->mmap_addr;
		multiboot::mmap_entry x;
		unsigned int offset = 0;
		terminal.print("Buffer size : %d\n", info->mmap_length); 
		unsigned int i = 0;
		for(; offset < info->mmap_length; ++i, offset += x.size + 4) {
			x = *(multiboot::mmap_entry*)(&ptr[offset]);
			memory_map_buffer[i].base =  x.base_addr;
			memory_map_buffer[i].length = x.length;
			memory_map_buffer[i].type = (decltype(memory_map_entry::type))x.type;
			
			terminal.print("offset %d : %p %p %d (%d)\n", offset, x.base_addr, x.length, x.type, x.size);
		}
		memory_map_buffer[i] = {0, 0, memory_map_entry::BADRAM};
	}
	/*
	   In the future we probably want to setup reporting of e.g. the memory map to the
	   main kernel using the multiboot info structure. For now we load directly the
	   main kernel.
	*/
	if(!(info->reported_flags & multiboot::info_flags::MODS) || info->module_count==0) {
		terminal.print("No module information found. Stopping.\n");
		halt();
	}
	if(!(info->reported_flags & multiboot::info_flags::CMDLINE)) {
		terminal.print("Can't determine name of kernel module. Stopping.\n");
		halt();
	}

	multiboot::module_decl* kernel_decl = nullptr;
	
	for(decltype(info->module_count) i = 0; i < info->module_count;++i) {
		if(strcmp(info->cmdline, info->module_arr_addr[i].cmdline) == 0) {
			kernel_decl = info->module_arr_addr + i;
		}
	}

	if(kernel_decl == nullptr) {
		terminal.print("Could not find kernel %s. Stopping.\n", info->cmdline);
		halt();
	}
	
	terminal.print("Kernel image end : %x\nBootstrapper code end : %x\nInfo struct ptr : %x\n",
			(int32_t)(kernel_decl->end),
			(int32_t)(&bootstrapper_end),
			(int32_t)(info));
	
	
	int32_t safe_offset = (int32_t)(kernel_decl->end) > (int32_t)(&bootstrapper_end) ?
				(int32_t)(kernel_decl->end) : (int32_t)(&bootstrapper_end);
	terminal.print("Safezone start : %x\n", safe_offset);
	auto r = load_kernel(safe_offset, kernel_decl->start);
	switch(r.result_code) {
		case lk_result::BAD_MAGIC:
			terminal.print("ELF magic number absent of kernel image. Stopping.");
			halt();break;
		case lk_result::BAD_ELF:
			terminal.print("Failed to parse kernel image. Stopping.");
			halt();break;
		case lk_result::NOT_EXEC:
			terminal.print("Kernel was not an executable file. Stopping.");
			halt();break;
		case lk_result::NOT_X64:
			terminal.print("Kernel was not compiled for x86-64. (got machine = %d). Stopping",
			r.diagnostic);
			halt();break;
		case lk_result::CANNOT_RELOC:
			terminal.print("Kernel tried to specify relocations. Stopping.");
			halt();break;
		case lk_result::SAFEZONE_VIOLATION:
			terminal.print("Kernel tried to load below the safe zone (or above 4GiB). Try linking it to higher addresses. Stopping.");
			halt();break;
		default:
			break;
	}
	
	terminal.print("Loaded kernel in memory.\n");

	unsigned int edx, dummy;	
	if((__get_cpuid(0x80000001, &dummy, &dummy, &dummy, &edx) == 0) || ((edx >> 29) & 0x1) == 0) {
		terminal.print(" 64 bit mode is not supported ! Stopping.\n");
		halt();
	}

	terminal.print("Kernel entry point : %x\n", (uint32_t)(r.entrypoint));
	ready_paging();
	early_gdt.code_segment.flags = 0b1010;
	return r.entrypoint;
}
