#include "pci.hh"
#include "x86/asm_stubs.hh"
#include "kstdlib/cstdio.hh"
namespace pci {

void enumerate() {
	for(int bus = 0; bus < 256;++bus) {
	for(int dev = 0; dev < 32; ++dev) {
		uint32_t conf_access = 0x80000000 | (bus << 16) | (dev << 11) | 0x8; 
		x64::outl(0xcf8, conf_access);
		uint16_t res = (x64::inl(0xcfc) >> 16) & 0xffff;
		if(res != 0xffff) {
			printf("Found device %x:%x of class %x\n", bus, dev, res);
			printf("Conf access %p\n", (uint64_t)(conf_access));
		}
	}
	}
}

}
