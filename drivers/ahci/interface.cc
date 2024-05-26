#include "interface.hh"
#include "kernel/kernel.hh"
#include "kstdlib/cstdio.hh"
namespace ahci {
f_pdrv_naive::f_pdrv_naive(int portnum, driver* parent) : internal(portnum, parent) {}


// So naïve it's not even funny
size_t f_pdrv_naive::read(void* buffer, size_t cnt) {
	if(cnt == 0) return 0;
	size_t begin_lba = pos >> 9; // 512 bytes per (logical) sector
	x64::phaddr page = get_phpage();
	auto vpage = (uint8_t*)(page - 512*1024*1024*1024ul);


	int to_rd = cnt < 4096 ? ((cnt+511)/512) : 4096/512;
	int c = internal.start_read((volatile void*)(vpage), begin_lba, to_rd);
	while(!internal.check_on_read(c));
	unsigned payload = cnt < (4096 - (pos%512)) ? cnt : (4096 - pos % 512);
	memcpy(buffer, (uint8_t*)(vpage) + (pos % 512), payload);
	pos += payload;
	release_phpage(page);
	if(payload != cnt) {
		return payload + read((uint8_t*)buffer + payload, cnt - payload);
	} else {return payload;}
};

size_t f_pdrv_naive::write(const void* buffer, size_t cnt) {
	if(cnt == 0) return 0;
	size_t begin_lba = pos >> 9;
	x64::phaddr page = get_phpage();
	auto vpage = (uint8_t*)(page - 512*1024*1024*1024ul);
	
	unsigned iter_size = (cnt < (4096 - pos%512)) ? cnt : (4096 - pos%512);
	// Useful payload is from pos%512 to (iter_size - 1) + pos%512
	int last_offset = iter_size - 1 + pos%512;

	int c1 = -1;
	int c2 = -1;
	if(pos % 512 != 0) {
		c1 = internal.start_read((volatile void*)(vpage), begin_lba, 1); // We'll write the 1st sector *back*
	}	
	if(last_offset % 512 != 511) {
		// We'll write part of the last sector back too.
		c2 = internal.start_read((volatile void*)(vpage + last_offset/512 * 512), begin_lba + last_offset/512, 1);
	}
	while(c1 != -1 && !internal.check_on_read(c1));
	while(c2 != -1 && !internal.check_on_read(c2));
	// Wait for synchro : then we can copy the actual buffer into the page.	
	memcpy((volatile void*)(vpage + (pos%512)), buffer, iter_size);
	int c3 = internal.start_write((volatile const void*)(vpage), begin_lba, (last_offset + 511)/512);
	while(!internal.check_on_read(c3)) {}	
	// We don't need to synchronise IF the HBA implementation ensures issue-order dispatch.
	// This is optional... If we want to be strict, use the synchro fence there.
	pos += iter_size;
	release_phpage(page);
	if(iter_size != cnt) {
		return iter_size + write((uint8_t*)buffer + iter_size, cnt - iter_size);
	} else {
		return iter_size;
	}
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
