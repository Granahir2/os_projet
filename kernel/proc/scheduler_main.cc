#include "proc.hh"
#include "kstdlib/cstdio.hh"
#include "mem/utils.hh"
#include "kernel/kernel.hh"

proc* toload; // Very naïve, to test.
extern "C" {
	regstate registers;
}

proc* current_process;

extern "C" {
void scheduler_main() {
	//static proc* currproc = new proc(); // Dummy, really
	//currproc->context = registers;
	registers = toload->context;
	ptss->ist[3] = (intptr_t)(&toload->kernel_stack[0]);
}

/*
Syscall semantics are like those of a function call, therefore there's no need 
to save 
*/
void syscall_main(int callnum, void* ptr, long arg1, long arg2, long arg3) {
	try {
	switch(callnum) {
		case 0: {
			auto* p = (filehandler*)(ptr);
			p->read((void*)arg1, arg2);} break;
		case 1: {
			auto* p = (filehandler*)(ptr);
			p->write((const void*)arg1, arg2);} break;
		default: break;
	}
	} catch(std::exception&) {}

	return;
}
}
