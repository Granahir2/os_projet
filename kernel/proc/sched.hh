#pragma once
#include "proc.hh"
#include "kstdlib/map.hh"
#include <cstdint>

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
	uint64_t how_long; // In fractions of cycle
	bool spin_after = false; // If true then, after the timeshare is exhausted, wait for the next cycle trigger to come.
};

struct graphnode {
    proc* process;
    uint64_t weight;
    uint64_t how_long;
    uint8_t balance;
    bool weight_fixed;
    bool new_process;
    map<uint16_t, graphnode*> forward_edges;
    map<uint16_t, graphnode*> backward_edges;

    graphnode(proc* p, uint64_t w, bool wf): process(p), weight(w), balance(0), weight_fixed(wf), new_process(true) {}
};

struct graphnode_list {
    graphnode* node;
    graphnode_list* next;

    graphnode_list(graphnode* n): node(n), next(nullptr) {}
    ~graphnode_list();
};


using rb_node = node<pair<uint16_t, graphnode*>>;

class hl_sched {
public:
    command next();
    void update_weights();
    void add_process(proc* p, uint64_t weight = -1, bool weight_fixed = false);
    void remove_process(proc* p);
    void add_edge(uint16_t pid1, uint16_t pid2);
    void remove_edge(uint16_t pid1, uint16_t pid2);
    void exec_report(bool graceful_yield);

    hl_sched(uint64_t ini_weight = 1'000'000'000):
                pid_to_node_pointer(), 
                visited(),
                rb_tree_root(pid_to_node_pointer.tree.root), 
                ready_queue_head(nullptr), 
                ready_queue_tail(nullptr), 
                ready_queue_iterator(nullptr), 
                graph_is_up_to_date(true), 
                weights_are_up_to_date(true),
                waiting_for_report(false),
                cycle_counter(0),
                initial_weight(ini_weight) {}
private:
    map<uint16_t, graphnode*> pid_to_node_pointer;
    map<uint16_t, bool> visited;
    map<uint16_t, bool> in_recursion_stack;

    graphnode_list* ready_queue_head;
    graphnode_list* ready_queue_tail;
    graphnode_list* ready_queue_iterator;
    graphnode* current_node;
    uint16_t cycle_counter;
    uint64_t initial_weight;

    bool graph_is_up_to_date;
    bool weights_are_up_to_date;
    bool waiting_for_report;

    void add_to_ready_queue_at_the_front(graphnode* node);
    void delete_queue();

    void topological_sort();
    rb_node* rb_tree_root;
    void dfs(rb_node* n);
    void dfs_over_forward_edges(rb_node* n);
};
