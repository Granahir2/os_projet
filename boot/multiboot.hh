/* Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 *  DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Modern C++ reimplementation of reference header multiboot.h of the GNU Multiboot 1
specification. (https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh)
*/

#pragma once

namespace multiboot {
// How far we search for the multiboot header as well as its required alignment
constexpr int search = 8192;
constexpr int alignment = 4;

static_assert(sizeof(intptr_t) == sizeof(uint32_t), "You do not seem to be targeting a 32 bit platform");

// Used to identify the header as well as for the OS to check it was correctly loaded
constexpr uint32_t header_magic = 0x1BADB002;
constexpr uint32_t bootloader_magic = 0x2BADB002; // Put in %eax

constexpr int module_alignment = 0x1000;
constexpr int info_struct_alignment = 4;

enum header_flags : uint32_t {PAGE_ALIGN = 1, MEMORY_INFO = 2, VIDEO_MODE = 4, AOUT_KLUDGE = 0x10000};
enum info_flags : uint32_t {MEMORY_BASIC = 0x1, BOOTDEV = 0x2, CMDLINE = 0x4, MODS = 0x8, // Do we have modules/etc info. Memory basic only tells us about low/hi mem
		   AOUT_SYMS = 0x10, ELF_SHDR = 0x20, // Do we have a symbol table/ELF section header table
		   MEMORY_FULL = 0x40, DRIVE = 0x80, CONFIG_TABLE = 0x100, BOOTLOADER_NAME = 0x200,
		   APM_TABLE = 0x400, VBE_INFO = 0x800, FRAMEBUFFER_INFO = 0x1000};

struct header {
	uint32_t magic = header_magic;
	uint32_t wanted_flags = 0;
	uint32_t checksum = 0; // The sum of the 3 fields defined above must be 0

	// If using feature AOUT_KLUDGE, tells the bootloader how to load the sections
	uint32_t header_addr = 0;
	uint32_t load_addr = 0;
	uint32_t load_end_addr = 0;
	uint32_t bss_end_addr = 0;
	uint32_t entry_addr = 0;

	// If using feature VIDEO_MODE, tells the bootloader what we want
	uint32_t mode_type = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 0;
};

// aout symbol tables won't be used ! Avoid using unions
// struct aout_symbol_table;

struct elf_section_header_table {
	// This is to be understood as an excerpt from an ELF file header
	uint32_t e_shnum;
	uint32_t e_shentsize;
	uint32_t e_shaddr; // Pointer to section header table
	uint32_t e_shstrndx; // String table
};

struct module_decl {
	void* start;
	void* end;
	char* cmdline;
	uint32_t padding;
};

struct mmap_entry {
	uint32_t size; // Does not count sizeof(size) !
	uint64_t base_addr;
	uint64_t length;
	enum {AVAILABLE, RESERVED, ACPI_RECLAIMABLE, NVS, BADRAM} type;
};

struct drive_entry {
	uint32_t size; // size of the full entry
	
	uint32_t drive_number;
	uint32_t drive_mode;
	uint32_t cylinders;
	uint32_t heads;
	uint32_t sectors;
	/* remaining data countains drive ports */
};

struct info { // Multiboot 1 information structure, given by the bootloader
	uint32_t reported_flags;

	uint32_t mem_lower; // Info given by flag MEMORY_BASIC.
	uint32_t mem_upper;

	uint32_t boot_device; // BOOTDEV
	char* cmdline; // CMDLINE

	uint32_t module_count; // MODS
	module_decl* module_arr_addr;

	elf_section_header_table elf_sec; // ELF_SHDR. Space would be claimed by AOUT_SYMS

	uint32_t mmap_length; // MEMORY_FULL		
	mmap_entry* mmap_addr;

	uint32_t drives_length; // DRIVES
	drive_entry* drives_addr;

	void* config_table; // CONFIG_TABLE
	char* bootloader_name; // BOOTLOADER_NAME

	/* Subsequent fields give info on APM, VBE and the framebuffer
	   They are not implemented as of now.
	   Note that the multiboot specification allows for new fields
	   to be defined : relying on sizeof(info) is a *bad* idea.
	*/
};

}
