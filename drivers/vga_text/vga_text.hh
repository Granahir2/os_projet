#pragma once
#include "x86/memory.hh"
#include "x86/asm_stubs.hh"
#include <cstddef>
#include "kernel/fs/fs.hh"
#include "kernel/fs/templates.hh"
#include "kstdlib/memory.hh"
/*
Port of the vga text driver found in misc/, for use with the VFS.
May get more fully fledged. Used with drivers/serial to showcase the
integration of drivers with devfs.
*/

namespace vga {
constexpr static x64::phaddr buff_addr  = 0xb8000;
constexpr static int ncol = 80;
constexpr static int nrow = 25;

enum color {
	BLACK = 0,
	BLUE = 1,
	GREEN = 2,
	CYAN = 3,
	RED = 4,
	MAGENTA = 5,
	BROWN = 6,
	LIGHT_GREY = 7,
	DARK_GREY = 8,
	LIGHT_BLUE = 9,
	LIGHT_GREEN = 10,
	LIGHT_CYAN = 11,
	LIGHT_RED = 12,
	LIGHT_MAGENTA = 13,
	LIGHT_BROWN = 14,
	WHITE = 15,
};

class text_driver;

class text_handle {
public:
	text_handle(text_driver* p) : parent(p) {}
	~text_handle();
	size_t write(const void* buffer, size_t cnt);
	size_t read(void* buffer, size_t cnt);

	int curr_color = 0x0f;
	text_driver* const parent;
private:
};

using file = perm_fh<vga::text_handle>;

class text_driver {
friend text_handle;
public:
	text_driver();
	operator bool();
	smallptr<file> get_file(int mode);
	
	void clear();
	
	unsigned int curr_col = 0;
	unsigned int curr_row = 0;    // Position on the screen
	unsigned int curr_offset = 0; // Offset in VGA memory.
private:
	void do_scroll();
	size_t refcnt = 0;
};
}
