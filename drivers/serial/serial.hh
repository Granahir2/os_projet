#pragma once
#include "x86/memory.hh"
#include <cstddef>
#include "kernel/fs.hh"

namespace serial {
class portdriver : public filehandler {
public:
	portdriver(uint16_t IOport = 0x3f8);
	bool test();
	
	size_t write(const void* buffer, size_t cnt) override;
	size_t read(void* buffer, size_t cnt) override;

	off_t seek(off_t offset, seekref whence) override; 
	statbuf stat() override;

	const uint16_t port;
};
}
