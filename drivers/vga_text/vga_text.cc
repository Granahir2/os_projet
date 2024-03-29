#include "vga_text.hh"

namespace vga {

text_adapter::text_adapter() {
	/*
	x64::outb(seq::aport, seq::MEMMODE)
	x64::outb(seq::dport, 0);
	
	x64::outb(seq::aport, seq::MAPMASK);
	x64::outb(seq::dport, 0xf);

	x64::outb(seq::aport, seq::CLOCKMODE);
	x64::outb(seq::dport, 0);
	*/
}

}
