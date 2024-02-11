#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <cpuid.h>

#include "multiboot.hh"
#include "x86/descriptors.hh"
#include "boot/load_kernel.hh"

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
	// Add a TSS ? 
} early_gdt; static_assert(sizeof(early_gdt) == 3*sizeof(x86::segment_descriptor));

constexpr x86::gdt_descriptor early_gdt_desc0 = {
	sizeof(early_gdt) - 1,
	(uint32_t)(&early_gdt)
}; // Weird kludge so that we don't *technically* convert &early_gdt to something
// other than a 32-bit reference...

extern "C" constexpr x64::gdt_descriptor early_gdt_desc = {early_gdt_desc0, 0};
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
	uint64_t loneentry;;
} level3_page_table;

extern "C" {

struct __attribute__((aligned(4096))) {
	uint64_t loneentry;
} level4_page_table;
}

void ready_paging() {
	level3_page_table.loneentry = (uint64_t)(&page_directory_table) | 0b11;
	level4_page_table.loneentry = (uint64_t)(&level3_page_table) | 0b11;
}
/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};
 
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
 
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}
 
size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;
 
void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}
 
void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}
 
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
 
void terminal_putchar(char c) 
{
	if(c == '\n') {
		terminal_column = 0;
		if(++terminal_row == VGA_HEIGHT) {terminal_row = 0;}
		return;
	}
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}
 
void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}
 
void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}

extern "C" void terminal_printhex(uint32_t d) {
	terminal_write("0x", 2);
	char str[9] = {'0','0','0','0','0','0','0','0','\0'};
	
	for(int i = 7; d != 0; i--) {
		if((d & 0xf) <= 9) {
			str[i] = '0'|(d & 0xf);
		} else {
			str[i] = 'a' - 10 + (d&0xf);
		}
		d >>=4;
	}
	for(int i = 0; i <= 7; ++i) {terminal_putchar(str[i]);}
}

int strcmp(const char* lhs, const char* rhs) {
	auto l= lhs, r = rhs;
	for(;*l==*r && (*l != '\0') && (*r != '\0'); l++,r++) {}
	return (*l - *r);
}

extern "C" void halt();
extern "C" const char bootstrapper_end; // linker defined symbol

extern "C" void* bootstrap(uint32_t magic, multiboot::info* info) {
	terminal_initialize();
	terminal_writestring("Hello World ! 64bit preloader build " __TIMESTAMP__ "\n");
	if(magic == multiboot::bootloader_magic) {
		terminal_writestring("Multiboot confirmed.\n");
	} else { halt();}
	/*
	   In the future we probably want to setup reporting of e.g. the memory map to the
	   main kernel using the multiboot info structure. For now we load directly the
	   main kernel.
	*/
	if(!(info->reported_flags & multiboot::info_flags::MODS) || info->module_count==0) {
		terminal_writestring("No module information found. Stopping.\n");
		halt();
	}
	if(!(info->reported_flags & multiboot::info_flags::CMDLINE)) {
		terminal_writestring("Can't determine name of kernel module. Stopping.\n");
		halt();
	}

	multiboot::module_decl* kernel_decl = nullptr;
	
	for(decltype(info->module_count) i = 0; i < info->module_count;++i) {
		if(strcmp(info->cmdline, info->module_arr_addr[i].cmdline) == 0) {
			kernel_decl = info->module_arr_addr + i;
		}
	}

	if(kernel_decl == nullptr) {
		terminal_writestring("Could not find kernel ");
		terminal_writestring(info->cmdline);
		terminal_writestring(". Stopping.\n");
	}
	
	terminal_writestring("Kernel image end : ");
	terminal_printhex((int32_t)(kernel_decl->end));
	terminal_writestring("\nBootstrapper code end : ");
	terminal_printhex((int32_t)(&bootstrapper_end));
	terminal_putchar('\n');
	
	
	int32_t safe_offset = (int32_t)(kernel_decl->end) > (int32_t)(&bootstrapper_end) ?
				(int32_t)(kernel_decl->end) : (int32_t)(&bootstrapper_end);
	auto r = load_kernel(safe_offset, kernel_decl->start);
	switch(r.result_code) {
		case lk_result::BAD_MAGIC:
			terminal_writestring("ELF magic number absent of kernel image. Stopping.");
			halt();break;
		case lk_result::BAD_ELF:
			terminal_writestring("Failed to parse kernel image. Stopping.");
			halt();break;
		case lk_result::NOT_EXEC:
			terminal_writestring("Kernel was not an executable file. Stopping.");
			halt();break;
		case lk_result::NOT_X64:
			terminal_writestring("Kernel was not compiled for x86-64. (got machine = ");
			terminal_printhex(r.diagnostic);
			terminal_writestring(" ).Stopping.");
			halt();break;
		case lk_result::CANNOT_RELOC:
			terminal_writestring("Kernel tried to specify relocations. Stopping.");
			halt();break;
		case lk_result::SAFEZONE_VIOLATION:
			terminal_writestring("Kernel tried to load below the safe zone (or above 4GiB). Try linking it to higher addresses. Stopping.");
			halt();break;
		default:
			break;
	}
	
	terminal_writestring("Loaded kernel in memory.\n");

	uint32_t edx, dummy;	
	if((__get_cpuid(0x80000001, &dummy, &dummy, &dummy, &edx) == 0) || ((edx >> 29) & 0x1) == 0) {
		terminal_printhex(edx);
		terminal_writestring(" 64 bit mode is not supported ! Stopping.\n");
		halt();
	}
	if(((edx >> 26) & 0x1)  == 0) {
		terminal_printhex(edx);
		terminal_writestring(" 1 GB pages not supported; cowardly bailing out\n");
	}

	terminal_writestring("Kernel entry point : ");
	terminal_printhex((uint32_t)(r.entrypoint));
	ready_paging();
	//reinterpret_cast<void(*)(void)>(r.entrypoint)();
	early_gdt.code_segment.flags = 0b1010;
	return r.entrypoint;
}
