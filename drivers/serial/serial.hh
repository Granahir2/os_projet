#pragma once
#include "x86/memory.hh"
#include <cstddef>
#include "kernel/fs/fs.hh"
#include "kernel/fs/templates.hh"
#include "kstdlib/memory.hh"
namespace serial {

class portdriver;
class porthandler;

class porthandler {
public:
	porthandler(portdriver* p); 
	~porthandler();
	
	size_t write(const void* buffer, size_t cnt);
	size_t read(void* buffer, size_t cnt);

	portdriver* const parent;	
};

using file = perm_fh<serial::porthandler>;

class portdriver {
friend porthandler;
public:
	portdriver(uint16_t IOport = 0x3f8);
	~portdriver();

	operator bool();
	smallptr<file> get_file(int mode);

	const uint16_t port;
private:
	size_t refcnt = 0;
};
}

