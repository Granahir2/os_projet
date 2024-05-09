#pragma once
#include "kernel/fs/fs.hh"
#include "kernel/fs/templates.hh"

namespace pci {

using addr_t = uint32_t;

struct cspace_fh {
	cspace_fh(addr_t addr);

	size_t read(void* buf, size_t cnt);
	size_t write(const void* buf, size_t cnt);
	off_t seek(off_t off, seekref whence);


	const unsigned pciaddr;
private:
	int curpos;
};

using csp_file = perm_fh<cspace_fh>;

class cspace {
public:
	smallptr<csp_file> get_file(addr_t addr, int mode);
};

};
