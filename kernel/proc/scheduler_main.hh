#pragma once
#include "sched.hh"

extern "C" void loadregs_hook();
extern "C" void syscall_hook(); 

extern "C" {
void scheduler_main();
void syscall_main(int callnum, void* ptr, long arg1, long arg2, long arg3);
}


extern hl_sched* scheduler;
extern proc* init;
extern proc* current_process;
