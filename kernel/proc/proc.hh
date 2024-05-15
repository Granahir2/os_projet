#pragma once

/*
Global pointer to currently running process -- used
so that the syscall interface can update it appropriately.
*/
//static proc* currproc = nullptr;

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
};

class proc {};

/*
class proc {
public:
	proc();
	bool exec(); // returns false if termination is requested.
	size_t getpid();
private:
	static size_t curr_PID; // Used for PID generation

	// Input buffers
	size_t argc;
	void** argv; // Everything is page-sized ?
	
	// Output buffers
	size_t outc;
	void** outv;

	size_t pid;
	regstate ring3_state;
	regstate ring0_state;
	bool is_in_syscall;
};*/