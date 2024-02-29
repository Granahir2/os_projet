/*   multiboot2.h - Multiboot 2 header file. */
/*   Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
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
/* Modern C++ reimplementation of GNU multiboot2.h header
*/

#pragma once

namespace multiboot2 {
constexpr int search = 32768;
constexpr int alignment = 8;

constexpr uint32_t header_magic = 0xe85250d6;
constexpr uint32_t bootloader_magic = 0x36d76289;

constexpr int module_alignment = 0x100;
constexpr int info_struct_alignment = 8; 

enum info_tag_types : uint32_t {
	end,
	cmdline,
	bootloader_name,
	mods,
	basic_meminfo,
	bootdev,
	mmap,
	vbe,
	framebuffer,
	elf_sections,
	apm,
	efi32,
	efi64,
	smbios,
	acpi_1, acpi_2,
	network,
	efi_mmap,
	efi_bs,
	efi32_ih,
	efi64_ih,
	load_base_addr
};

namespace header {
enum tag_type : uint16_t {
	end,
	info_request,
	address,       // Not needed for ELF images
	entry_address, // skipping definitions
	console_flags,
	framebuffer,
	module_align,
	efi_bs,
	entry_address_efi32, // Planning not to use EFI
	entry_address_efi64, // Skipping
	relocatable
};

template<int T>
struct inforeq_tag alignas(8) {
	constexpr uint16_t type = tag_type::info_request;
	uint16_t flags = 0;
	constexpr uint32_t size = T*sizeof(info_tag_type) + 8;
	info_tag_type requests[T];
};

struct consoleflags_tag alignas(8) {
	constexpr uint16_t type = tag_type::console_flags;
	uint16_t flags = 0;
	constexpr uint32_t size = 12;
	uint32_t console_flags = 0;
};

struct framebuffer_tag alignas(8) {
	constexpr uint16_t type = tag_type::framebuffer;
	uint16_t flags = 0;
	constexpr uint32_t size = 20;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 0;
};

struct modalign_tag alignas(8) {
	constexpr uint16_t type = tag_type::module_align;
	uint16_t flags = 0;
	uint32_t size = 8;
};

struct reloc_tag alignas(8) {
	constexpr uint16_t type = tag_type::relocatable;
	uint16_t flags = 0;
	uint32_t size = 8;
};
}
}

