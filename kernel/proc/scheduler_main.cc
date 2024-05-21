#include "proc.hh"
#include "kstdlib/cstdio.hh"
#include "mem/utils.hh"
#include "kernel/kernel.hh"
#include "sched.hh"


hl_sched* scheduler;

proc* init[3];

proc* current_process;

extern "C" {
	regstate registers;
}


extern "C" {
void scheduler_main() {
	static bool is_launched = false;

	puts("Entering scheduler");

	if(!is_launched) {
		scheduler = new hl_sched();
		scheduler->add_process(init[0]);
		//scheduler->add_process(init[1]);
		//scheduler->add_process(init[2]);
		current_process = init[0];
		is_launched = true;
	} else {
		current_process->context = registers;
		scheduler->exec_report(true);
	}

	try {
	auto com = scheduler->next();
	printf("com : pid = %lu, how_long = %lx, wait_after = %d\n", com.to_exec->get_pid(), com.how_long, com.spin_after);
	registers = com.to_exec->context;
	ptss->ist[3] = (intptr_t)(&com.to_exec->kernel_stack[0]);
	} catch(std::exception& e) {
		puts(e.what());
		halt();
	}
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
