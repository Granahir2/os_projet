#include "serial.hh"
#include "x86/asm_stubs.hh"
#include "kstdlib/stdexcept.hh"
#include "kstdlib/cstdio.hh"

namespace serial {

portdriver::portdriver(uint16_t IOport) : port(IOport), refcnt(0) {
	x64::outb(port+1, 0); // no interrupts

	x64::outb(port+3, 0x80);
	x64::outb(port+0, 0x01); // 115200 baud
	x64::outb(port+1, 0);

	x64::outb(port+3, 0x03); // 8n1
	x64::outb(port+2, 0xe1); // 64 bit buffer 
}

portdriver::~portdriver() {
	if(refcnt) {
		printf("[error][serial] Tried to destroy driver but there where still open files."); 
	}
}

portdriver::operator bool() {
	// TODO test for existence of port
	return refcnt > 0;
}



smallptr<file> portdriver::get_file([[maybe_unused]] int mode) {
	return smallptr<file>(new serial::file(mode, this));
}

porthandler::~porthandler() {
	if(parent && parent->refcnt) {
		parent->refcnt--;
	}
}

size_t porthandler::write(const void* buf, size_t count) {
	size_t i = 0;
	for(; i < count && x64::inb(parent->port+5)&0x20; ++i) {
		x64::outb(parent->port+0, ((const uint8_t*)(buf))[i]);
	}
	return i;
}

size_t porthandler::read(void* buf, size_t cnt) {
	size_t c = 0;
	auto p = (unsigned char*)(buf);
	for(;c < cnt && (x64::inb(parent->port+5)&0x1); c++, p++) {*p = x64::inb(parent->port);}
	return c;
}

}

