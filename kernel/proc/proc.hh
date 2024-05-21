#pragma once
#include <cstdint>
#include "x86/memory.hh"
#include "kernel/fs/fs.hh"

/*
Naïve abstraction of a process at the lowest level :
inputs, outputs, and ways to context switch.
*/

struct alignas(64) __attribute__((packed)) regstate {
	uint64_t gp_regs[16];
	uint8_t fpu_state[512];
	
	uint64_t flags;
	uint64_t rip;
	uint64_t cs;
	uint64_t ss;
	
	x64::phaddr cr3;
};

struct flist {
	filehandler* pl;
	flist* next = nullptr;
};

class proc {
public:
	proc(filehandler* loadfrom, filehandler* stdo = nullptr, filehandler* stdi = nullptr); // loads an ELF executable
	void allocate_buffers(x64::phaddr* table, size_t howmany);

	size_t get_pid();

	regstate context;
	uint8_t kernel_stack[4096];

	flist* open_files;
		
	int bufnumber = 0; // Buffers are shared memory
private:
	static size_t c_pid;
	size_t pid;
};

extern proc* toload;
