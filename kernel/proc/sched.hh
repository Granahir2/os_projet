#pragma once
#include "proc.hh"

/***************************************
* Scheduler's logic. The hl_sched class (highlevel scheduling) should have :
*
* - a way to specify a graph of process to start. It may be mutable or
* immutable, (a new hl_sched is spawned at each new graph); your choice.
* It needs to support a way for the user to optionally set weights.
* Weights are fractional and represent 1/2^32 of a cycle. (maybe we reserve like 10% for the OS/ctx switches ?)
* 
* -  A function `command hl_sched::step()` which decides how execution should proceed; see below.
*
* -  A function `void hl_sched::update_weights()` which updates the scheduling heuristics.
*
* The kernel is a "client" of this class : after constructing the graph of processes,
* it will repeatedly call step() to know what to do next and do it.
*
* This design is such that ctx switching details are abstracted from hl_sched and scheduling strategy
* is abstracted from the rest of the kernel... If you find this is broken in some way please tell me.
*
* It should be doable to test hl_sched on regular Linux by creating a dummy proc class and using the code
* here as a regular Linux source. Might help if the technical part gets late.
***************************************/

struct command {
	proc* to_exec;
	long int how_long; // In fractions of cycle
	bool spin_after = false; // If true then, after the timeshare is exhausted, wait for the next cycle trigger to come.
};

