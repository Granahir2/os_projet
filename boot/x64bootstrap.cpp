#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <cpuid.h>

#include "multiboot.hh"
#include "x86/descriptors.hh"
#include "boot/load_kernel.hh"
#include "misc/display.h"

constexpr multiboot::header craft_multiboot_header() {
	multiboot::header header;
	header.magic = multiboot::header_magic;
	header.wanted_flags = multiboot::header_flags::PAGE_ALIGN | multiboot::header_flags::MEMORY_INFO;
	header.checksum = -header.magic - header.wanted_flags;
	return header;
};
extern "C" __attribute__((section(".multiboot")))
constexpr multiboot::header multiboot_header = craft_multiboot_header();

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


struct __attribute__((aligned(4096))) {
	uint64_t entry0;
	uint64_t entry1;
	uint64_t entry2;
	uint64_t entry3;
} level3_page_table;

extern "C" {

struct __attribute__((aligned(4096))) {
	uint64_t loneentry;
} level4_page_table;
}

void ready_paging() {
	level3_page_table.entry0 = (uint64_t)(&page_directory_table.dir0) | 0b11;
	level3_page_table.entry1 = (uint64_t)(&page_directory_table.dir1) | 0b11;
	level3_page_table.entry2 = (uint64_t)(&page_directory_table.dir2) | 0b11;
	level3_page_table.entry3 = (uint64_t)(&page_directory_table.dir3) | 0b11;
	level4_page_table.loneentry = (uint64_t)(&level3_page_table) | 0b11;
}

int strcmp(char* a, char* b) {
	auto c = a;
	auto d = b;
	for(;*c != *d && *c != '\0' && *d != '\0'; c++,d++) {}
	return *d - *c;
}

extern "C" void halt();
extern "C" const char bootstrapper_end; // linker defined symbol

extern "C" void* bootstrap(uint32_t magic, multiboot::info* info) {
	Display terminal = Display();
	terminal.print("Hello World ! 64bit preloader build " __TIMESTAMP__ "\n");
	if(magic == multiboot::bootloader_magic) {
		terminal.print("Multiboot confirmed.\n");
	} else { halt();}
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
	
	terminal.print("Kernel image end : %x\nBootstrapper code end : %x\n",
			(int32_t)(kernel_decl->end),
			(int32_t)(&bootstrapper_end));
	
	
	int32_t safe_offset = (int32_t)(kernel_decl->end) > (int32_t)(&bootstrapper_end) ?
				(int32_t)(kernel_decl->end) : (int32_t)(&bootstrapper_end);
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

	uint32_t edx, dummy;	
	if((__get_cpuid(0x80000001, &dummy, &dummy, &dummy, &edx) == 0) || ((edx >> 29) & 0x1) == 0) {
		terminal.print(" 64 bit mode is not supported ! Stopping.\n");
		halt();
	}

	terminal.print("Kernel entry point : %d\n", (uint32_t)(r.entrypoint));
	ready_paging();
	early_gdt.code_segment.flags = 0b1010;
	return r.entrypoint;
}
