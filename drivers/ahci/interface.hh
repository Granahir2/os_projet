#pragma once
#include "ahci.hh"
#include "kernel/fs/fs.hh"
#include "kernel/fs/templates.hh"
namespace ahci {

class f_pdrv_naive {
public:
	f_pdrv_naive(int portnum, driver* parent);
	size_t read(void* buffer, size_t cnt);
	size_t write(const void* buffer, size_t cnt);
	off_t  seek(off_t offset, seekref whence);
private:
	port_drv internal;
	size_t pos;
};

using file = perm_fh<f_pdrv_naive>;

/*
struct buf_entry {
	size_t lba_pos;	
	x64::phaddr ptr = mem::phalloc_null;
	int is_present;
	uint16_t numsector;
};

class buffered_pdrv { // Maybe integrated with the scheduler for async-ness.
public:

private:
	buf_entry rolling_cache[256]; // Oldest values are those farthest from the index
	int cache_index = 0;
	int drive_index = 0;
}
*/

}
