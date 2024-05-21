#include "sched.hh"
#include "proc.hh"
#include "kstdlib/new.hh"
#include "kstdlib/map.hh"
#include "kstdlib/stdexcept.hh"
#include "kstdlib/cstdio.hh"

graphnode_list::~graphnode_list() {
    if (next != nullptr) {
        next->~graphnode_list();
    }
    delete next;
}

void hl_sched::add_process(proc* p, uint64_t weight, bool weight_fixed) {
    if(weight == -1ull) {
	    weight = initial_weight;	
    }
    graphnode* new_node = new graphnode(p, weight, weight_fixed);
    uint16_t pid = p->get_pid();
    pid_to_node_pointer[pid] = new_node;
    
    graph_is_up_to_date = false;
    weights_are_up_to_date = false;
    rb_tree_root = pid_to_node_pointer.tree.root;
}

void hl_sched::remove_process(proc* p) {
    uint16_t pid = p->get_pid();
    delete pid_to_node_pointer[pid];
    pid_to_node_pointer.erase(pid);
    
    graph_is_up_to_date = false;
    weights_are_up_to_date = false;
}

void hl_sched::add_edge(uint16_t pid1, uint16_t pid2) {
    graphnode* node1 = pid_to_node_pointer[pid1];
    graphnode* node2 = pid_to_node_pointer[pid2];
    node1->forward_edges[pid2] = node2;
    node2->backward_edges[pid1] = node1;
    
    graph_is_up_to_date = false;
}
void hl_sched::remove_edge(uint16_t pid1, uint16_t pid2) {
    graphnode* node1 = pid_to_node_pointer[pid1];
    graphnode* node2 = pid_to_node_pointer[pid2];
    node1->forward_edges.erase(pid2);
    node2->backward_edges.erase(pid1);

    graph_is_up_to_date = false;
}

void hl_sched::exec_report(bool graceful_yield) {
    if (!waiting_for_report) {
        throw runtime_error("Not waiting for report");
    }
    if (graceful_yield & (!current_node->weight_fixed)) {
        current_node->weight++;
        weights_are_up_to_date = false;
    }
    waiting_for_report = false;
}

// Calculate the integer square root of x,
// from https://web.archive.org/web/20120306040058/http://medialab.freaknet.org/martin/src/sqrt/sqrt.c
uint16_t isqrt(uint8_t x) {
    uint16_t op, res, one;
    op = x;
    res = 0;
    /* "one" starts at the highest power of four <= than the argument. */
    one = 1 << 10;  /* second-to-top bit set */
    while (one > op) one >>= 2;
    while (one != 0) {
        if (op >= res + one) {
            op -= res + one;
            res += one << 1;  // <-- faster than 2 * one
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

void hl_sched::update_weights() {
    if (weights_are_up_to_date) return;
    if (ready_queue_head == nullptr) 
        throw runtime_error("Ready queue is empty");
    uint64_t total_sqrt_balance = 0;
    uint64_t total_weight = 0;
    uint64_t total_time = 0;

    // Magic algorithm
    for (graphnode_list* it = ready_queue_head; it != nullptr; it = it->next) {
        graphnode* node = it->node;
        if (node->weight_fixed) continue;
        if (node->new_process) node->balance = cycle_counter >> 1, node->new_process = false;
        node->balance = isqrt(node->balance);
        total_sqrt_balance += node->balance;
    }
    // When the total balance is 0, there is no information about the processes
    // In fact, they should be running perfectly, and we don't need to do any update
    if (total_sqrt_balance == 0) return;
    
    for (graphnode_list* it = ready_queue_head; it != nullptr; it = it->next) {
        graphnode* node = it->node;
        if (node->weight_fixed) continue;
        node->weight = node->weight / total_sqrt_balance * node->balance;
        total_weight += node->weight;
        node->balance = 0;
    }
    
    // Calculate the time for each process, proportional to its weight
    for (graphnode_list* it = ready_queue_head; it != nullptr; it = it->next) {
        graphnode* node = it->node;
        node->how_long = node->weight / total_weight * (1ll<<32);
        total_time += node->how_long;
    }
    
    // If the total time is not 2^32, then we need to adjust the last process
    if (total_time != (1ll<<32)) {
        ready_queue_tail->node->how_long += (1ll<<32) - total_time;
    }
    cycle_counter = 0;
    weights_are_up_to_date = true;
}

void hl_sched::add_to_ready_queue_at_the_front(graphnode* node) {
    printf("Adding process %d to the ready queue\n", node->process->get_pid());
    graphnode_list* new_node = new graphnode_list(node);
    if (ready_queue_head == nullptr) {
        ready_queue_head = new_node;
        ready_queue_tail = new_node;
    } else {
        new_node->next = ready_queue_head;
        ready_queue_head = new_node;
    }
}

void hl_sched::delete_queue() {
    while(ready_queue_head != nullptr) {
        graphnode_list* next = ready_queue_head->next;
        delete ready_queue_head;
        ready_queue_head = next;
    }
    ready_queue_tail = nullptr;
    ready_queue_iterator = nullptr;
}

command hl_sched::next() {
    if (waiting_for_report) {
        throw runtime_error("Not ready for next command, waiting for report");
    }
    if (ready_queue_iterator == nullptr) {
        if (!graph_is_up_to_date)
        {
            // Delete the old ready queue
            delete_queue();

            // Build the ready queue
            dfs(rb_tree_root);

            visited.clear();
            in_recursion_stack.clear();
            graph_is_up_to_date = true;
        }
        // Update the weights if necessary
        update_weights();

        ready_queue_iterator = ready_queue_head;
    }
    if (ready_queue_iterator == nullptr) {
        throw runtime_error("Ready queue is empty");
    }
    graphnode* current_node = ready_queue_iterator->node;
    ready_queue_iterator = ready_queue_iterator->next;
    command c;
    c.to_exec = current_node->process;
    if (ready_queue_iterator == nullptr) {
        c.spin_after = true;
        cycle_counter++;
    } else {
        c.spin_after = false;
    }
    c.how_long = current_node->how_long;
    waiting_for_report = true;
    current_node = current_node;
    return c;
}

void hl_sched::dfs(rb_node* n) {
    if (n == nullptr) return;
    uint16_t pid = n->value.key;
    if (visited.contains(pid) && visited[pid]) return;
    visited[pid] = true;
    if (in_recursion_stack.contains(pid) && in_recursion_stack[pid]) 
        throw runtime_error("Cycle detected in the scheduler's graph of processes");
    in_recursion_stack[pid] = true;
    
    // DFS over the graph
    graphnode* node = n->value.value;
    if (!node->forward_edges.empty()) 
        dfs_over_forward_edges(node->forward_edges.tree.root);
    add_to_ready_queue_at_the_front(node);
    in_recursion_stack[pid] = false;

    // DFS over the rest of the red-black tree
    dfs(n->left);
    dfs(n->right);
}

void hl_sched::dfs_over_forward_edges(rb_node* n) {
    if (n == nullptr) return;
    uint16_t pid = n->value.key;
    if (visited.contains(pid) && visited[pid]) return;
    visited[pid] = true;
    if (in_recursion_stack.contains(pid) && in_recursion_stack[pid]) 
        throw runtime_error("Cycle detected in the scheduler's graph of processes");
    in_recursion_stack[pid] = true;
    dfs(n);
    add_to_ready_queue_at_the_front(n->value.value);
    in_recursion_stack[pid] = false;

    // DFS over the rest of the red-black tree
    dfs_over_forward_edges(n->left);
    dfs_over_forward_edges(n->right);
}
