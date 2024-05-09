#include "interface.hh"
#include "kernel/kernel.hh"
#include "kstdlib/cstdio.hh"
namespace ahci {
f_pdrv_naive::f_pdrv_naive(int portnum, driver* parent) : internal(portnum, parent) {}


// So naïve it's not even funny
size_t f_pdrv_naive::read(void* buffer, size_t cnt) {
	size_t begin_lba = pos >> 9; // 512 bytes per (logical) sector
	x64::phaddr page = get_phpage();
	int c = internal.start_read((volatile void*)(page), begin_lba, 4096/512);
	while(!internal.check_on_read(c));
	//printf("Read success\n");
	int payload = cnt < (4096 - (pos%512)) ? cnt : 4096 - (pos % 512);
	memcpy(buffer, (uint8_t*)(page) + (pos % 512), payload);
	pos += payload;
	release_phpage(page);
	if(payload != cnt) {
		return payload + read((uint8_t*)buffer + payload, cnt - payload);
	} else {return payload;}
};

size_t f_pdrv_naive::write(const void* buffer, size_t cnt) {
	throw eperm("Hard disks are RO for now.");
}

off_t f_pdrv_naive::seek(off_t off, seekref whence) {
	switch(whence) {
		case SET:
			if(off < 0 || (size_t)(off) > internal.total_size*512) {
				printf("Tried to access offset %ld\n", off);
				throw einval("[ahci] Out of bounds seek\n");
			} else {
				return (off_t)(pos = (size_t)off);
			}
		case CUR:
			return seek(off + pos, SET); 
		case END:
			return seek(internal.total_size*512 - off, SET);
		default:
			throw einval("Illegal seek");
	}
}

}
