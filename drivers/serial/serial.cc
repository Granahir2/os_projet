#include "serial.hh"
#include "x86/asm_stubs.hh"

namespace serial {
portdriver::portdriver(uint16_t IOport) : port(IOport) {
	x64::outb(port+1, 0); // no interrupts

	x64::outb(port+3, 0x80);
	x64::outb(port+0, 0x01); // 115200 baud
	x64::outb(port+1, 0);

	x64::outb(port+3, 0x03); // 8n1
	x64::outb(port+2, 0xe1); // 64 bit buffer 
}

size_t portdriver::write(const void* buf, size_t count) {
	size_t i = 0;
	for(; i < count && x64::inb(port+5)&0x20; ++i) {
		x64::outb(port+0, ((const uint8_t*)(buf))[i]);
	}
	return i;
}

size_t portdriver::read(void* buf, size_t cnt) {
	size_t c = 0;
	auto p = (unsigned char*)(buf);
	for(;c < cnt && (x64::inb(port+5)&0x1); c++, p++) {*p = x64::inb(port);}
	return c;
}

off_t portdriver::seek(off_t, seekref) {return 0;}
statbuf portdriver::stat() {return statbuf();}
}
