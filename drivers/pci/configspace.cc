#include "configspace.hh"
#include "x86/asm_stubs.hh"
#include "kstdlib/cstdio.hh"
namespace pci {
cspace_fh::cspace_fh(addr_t addr) : pciaddr(addr << 8), curpos(0)  {

	uint16_t vendor;  
	read(&vendor, 2);
	if(vendor == 0xffff) {
		throw einval("No device at specified address");
	}
	seek(0, SET);
};

size_t cspace_fh::read(void* buf, size_t cnt) {
	uint32_t cword = 0;
	unsigned y = 1;

	unsigned c = 0;
	for(; c < cnt && curpos < 256; ++c) {
		unsigned x = curpos & 0b11111100;
		if(x != y) {
			y = x;
			x64::outl(0xcf8, y | pciaddr | 0x80000000u);
			cword = x64::inl(0xcfc);
		}
		((uint8_t*)(buf))[c] = (cword >> ((curpos - y)*8)) & 0xff;
		curpos++;
	}
	return c;
}

size_t cspace_fh::write(const void* buf, size_t cnt) {
	uint32_t cword;
	if(curpos & 0b11) {read(&cword, sizeof(cword));}

	unsigned c = 0;
	for(; c < cnt && curpos < 256; ++c) {
		auto modpos = curpos & 0b11;
		auto wordpos = curpos & 0b11111100;

		cword = (cword & ~(0xff << (modpos*8))) | (((uint8_t*)buf)[c] << (modpos*8));
		
		if(((curpos++) & 0b11) == 0) {
			x64::outl(0xcf8, wordpos | pciaddr | 0x80000000u); 
			x64::outl(0xcfc, cword);
		}
	}

	if((curpos & 0b11) != 0) { // We have not yet committed the remaining changes
		auto wordpos = curpos & 0b11111100;
		auto modpos = curpos & 0b11;
		uint32_t buf;
		read(&buf, sizeof(buf));

		cword &= ~(0xffffffffu << (modpos*8)); // Blank the higher bytes
		buf   &= ~(0xffffffffu >> ((4 - modpos)*8)); // Blank the lower bytes
		cword |= buf;
		x64::outl(0xcf8, wordpos + pciaddr + 0x80000000u);
		x64::outl(0xcfc, cword);
	}
	return c;
}

off_t cspace_fh::seek(off_t off, seekref whence) {
	switch(whence) {
		case SET:
			if(off < 0 || off > 64) {throw einval("Out of bounds seek");}
			curpos = (decltype(curpos))(off);
			return curpos;
		case CUR:
			return seek(curpos + off, SET);
		case END:
			return seek(64 - off, END);
		default:
			throw einval("Invalid seek position.");
	}
}

smallptr<csp_file> cspace::get_file(addr_t addr, int mode) {
	return smallptr<csp_file>(new csp_file(mode, addr));
}
}
