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
	if(!is_launched) {
		scheduler = new hl_sched();
		scheduler->add_process(init[0]);
		scheduler->add_process(init[1]);
		scheduler->add_edge(init[0]->get_pid(), init[1]->get_pid());
		scheduler->add_process(init[2]);
		scheduler->add_edge(init[1]->get_pid(), init[2]->get_pid());
		current_process = init[0];
		is_launched = true;
	} else {
		current_process->context = registers;
		/*for(int i = 0; i < 256/32; ++i) {
			printf("ISR : %x\n", pimngr->apic_base()[0x100/4 + 4*i]);
			// Every word is 16 byte aligned
		}*/
		bool has_been_interrupted = pimngr->apic_base()[0x100/4 + 4] & (0b100); // We are vector 34
		if(has_been_interrupted) {puts("Ran out of time!");/*pimngr->send_EOI();*/}
		scheduler->exec_report(!has_been_interrupted);
	}
	pimngr->send_EOI();

	/*
	puts("---------------------");
	for(int i = 0; i < 256/32; ++i) {
		printf("ISR : %x\n", pimngr->apic_base()[0x100/4 + 4*i]);
	}*/
	
	try {
	auto com = scheduler->next();
	printf("com : pid = %lu, how_long = %lx, wait_after = %d\n", com.to_exec->get_pid(), com.how_long, com.spin_after);
	registers = com.to_exec->context;
	current_process = com.to_exec;
	ptss->ist[3] = (intptr_t)(&com.to_exec->kernel_stack[0]);



	pimngr->apic_base()[0x320/4] &= ~(1ul << 16);// Setup APIC on "base" interrupt;
	pimngr->apic_base()[0x3e0/4] = 0;
	pimngr->apic_base()[0x380/4] = (unsigned int)(com.how_long);

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
