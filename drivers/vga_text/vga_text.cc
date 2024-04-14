#include "vga_text.hh"
#include "kstdlib/cstdio.hh"
namespace vga {
size_t text_handle::write(const void* buf, size_t cnt) {
	auto p = (unsigned char*)buf;
	for(size_t i = 0; i < cnt; ++i) {
		char c = p[i];
		auto q = (uint16_t *)(buff_addr) + (parent->curr_col + parent->curr_row*ncol + parent->curr_offset);
		switch(c) {
			default:
				*q = (uint16_t)(c) | (curr_color << 8);
				if(++(parent->curr_col) != ncol) { break; }
			[[fallthrough]];
			case '\n':
				parent->do_scroll();
		}
	}
	return cnt;
}
size_t text_handle::read(void*, size_t) {return 0;}

text_handle::~text_handle() {
	if(parent && parent->refcnt) {parent->refcnt--;}
}

void text_driver::do_scroll() {
	curr_col = 0;
	if(++curr_row == nrow) {
		curr_row = nrow - 1;
		curr_offset += ncol;
		curr_offset %= (1 << 14); // Use 16 KB / plane mode

		// Scroll by one line
		x64::outb(0x3d4, 0x0d);
		x64::outb(0x3d5, curr_offset & 0xff);
		x64::outb(0x3d4, 0x0c);
		x64::outb(0x3d5, (curr_offset >> 8) & 0xff);
	
		// Clear the line we just displayed at the bottom of the screen
		uint16_t last_line_off = curr_offset + (nrow - 1)*ncol;
		memset((uint16_t *)(buff_addr) + last_line_off, 0, nrow*sizeof(uint16_t));
	}
}

smallptr<file> text_driver::get_file(int mode) {
	refcnt++;
	return smallptr<file>(new file(mode, this));
}
text_driver::operator bool() {
	return refcnt > 0;
}
text_driver::text_driver() {
	x64::outb(0x3c4, 0x04);
	x64::outb(0x3c5, 0x00); // disable extended memory
}
void text_driver::clear() {
	curr_col = curr_row = curr_offset = 0;
	memset((uint16_t *)buff_addr, 0, nrow*ncol*sizeof(uint16_t));
}
}
