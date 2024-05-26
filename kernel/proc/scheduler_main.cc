#include "proc.hh"
#include "kstdlib/cstdio.hh"
#include "mem/utils.hh"
#include "kernel/kernel.hh"
#include "sched.hh"

#include <algorithm>
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
		pimngr->send_EOI();
	} else {
		current_process->context = registers;
		bool has_been_interrupted = pimngr->apic_base()[0x100/4 + 4] & (0b100); // We are vector 34
		if(has_been_interrupted) {pimngr->send_EOI();}
		scheduler->exec_report(!has_been_interrupted);
	}
	
	try {
	auto com = scheduler->next();
	//printf("com : pid = %lu, how_long = %lx, wait_after = %d\n", com.to_exec->get_pid(), com.how_long, com.spin_after);
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
long syscall_main(int callnum, void* ptr, long arg1, long arg2, long arg3) {
	try {
	switch(callnum) {
		case 0: { // read
			auto* p = (filehandler*)(ptr);
			if(!p) {return -1;}
			return p->read((void*)arg1, arg2);} break;
		case 1: { // write
			auto* p = (filehandler*)(ptr);
			if(!p) {return -1;}
			return p->write((const void*)arg1, arg2);} break;
	
		case 2: { // open
			auto* p = (dir_iterator*)(ptr);
			if(!p) {return -1;}
			auto x = p->open_file((const char*)(arg1), arg2);
			if(x.ptr) {
				current_process->open_files = new flist{x.ptr, current_process->open_files};
			} return (long)x.ptr;};
		case 3: { // traverse
			auto* p = (dir_iterator*)(ptr);
			if(!p) {return -1;}
			return (*p << (const char*)(arg1));}
		case 4: { // grabs a vfs iterator
			auto* d = vfs->get_iterator();
			if(d) {
				current_process->open_dirs = new dlist{d, current_process->open_dirs};
			} return (long)d;}
		case 5: { // gets canonical path
			auto* d = (dir_iterator*)(ptr);
			if(!d) {return -1;}
			strncpy((char*)arg1, d->get_canonical_path().c_str(), arg2);
			return 0;
		}
		case 6: {
			auto* d = (dir_iterator*)(ptr);
			if(!d) {return -1;}
			try {
			dirlist_token dirtok = d->list();
			dirlist_token* index = &dirtok;

			size_t remaining = (long)arg2;
			if(!remaining) {return -1;}
			char*  result = (char*)arg1;
			while(index != nullptr && remaining > 0) {
				strncpy(result, index->name.c_str(), remaining - 1);
				size_t m = std::min(strlen(index->name.c_str()), remaining - 1);
				result+=m;
				*result = '\0';
				result++;
				if(m == remaining - 1) { // We're over	
					remaining = 0;
				} else {
					remaining -= m + 1;
				}
				index = index->next.ptr;
			}
			return (long)arg2 - remaining;} catch(empty_directory&) {
				return 0;
			}
		}

		default: break;
		}
	} catch(std::exception&) {}

	return -1ull;
}
}
