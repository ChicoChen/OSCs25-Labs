#include "thread/thread.h"
#include "exception/exception.h"
#include "allocator/dynamic_allocator.h"
#include "allocator/page_allocator.h"
#include "template/queue.h"
#include "timer/timer.h"
#include "memory_region.h"
#include "basic_type.h"
#include "utils.h"

#include "mini_uart.h"
#include "str_utils.h"

static Queue run_queue;
static Queue wait_queue;
static Queue dead_queue;

static Thread *idle_thread = NULL;
static Thread *systemd = NULL;

static uint32_t total_thread = 0;
static uint64_t switch_interval = ~0;

// ----- forward decls -----

void thread_init(Thread *target_thread, Task assigned, void *args, RCregion *prog);
void thread_entry(void *, uint64_t *state);
bool thread_alive(Thread* thread);

void launch_user_process(void *prog_entry);

void switch_context(void *arg, uint64_t *trap_frame);
void enable_context_swtich();

void idle();
void kill_zombies();

// defined in __switch_thread()
extern void switch_to(uint64_t *prev, uint64_t *next);
// defined in __switch_thread()
extern uint64_t* get_curr_thread_state();

// ----- public interface -----

void init_thread_sys(){
    queue_init(&run_queue);
    queue_init(&wait_queue);
    queue_init(&dead_queue);

    uint64_t count, freq;
    get_timer(&count, &freq);
    switch_interval = freq >> 5;

    idle_thread = (Thread *)dyna_alloc(THREAD_SIZE);
    thread_init(idle_thread, idle, NULL, NULL);
    
    systemd = (Thread *)dyna_alloc(THREAD_SIZE);
    thread_init(systemd, NULL, NULL, NULL);
    systemd->excepts = get_curr_workload();
    asm volatile("msr tpidr_el1, %[init_thread]" : :[init_thread]"r"(systemd->thread_state) :);
}

Thread *make_thread(Task assigned_func, void *args, RCregion* prog){
    Thread *new_thread = (Thread *)dyna_alloc(THREAD_SIZE);
    thread_init(new_thread, assigned_func, args, prog);
    
    // insert into queue
    node_init(&new_thread->node);
    queue_push(&run_queue, &new_thread->node);
    if(total_thread == 3){ // as the first thread added
        enable_context_swtich();
    }
    return new_thread;
}

void create_prog_thread(RCregion *prog_entry){
    // 1. create thread which start from given address in el0
    // 2. schedule to execute it, leave memory reclaiming to idle thread
    make_thread(launch_user_process, NULL, prog_entry);
}

/// @brief Schedule next thread and jump to execute it.
/// @note Upon calling, won't return until current thread is scheduled to execute again.
void schedule(){
    // get current thread and next thread
    Thread *preemptee = get_curr_thread();
    if(preemptee->id > 1 && thread_alive(preemptee)){
        queue_push(&run_queue, &preemptee->node);
    }   
    
    ListNode *head = queue_pop(&run_queue);
    Thread *preemptor = (!head)? idle_thread: GET_CONTAINER(head, Thread, node);
    // if(preemptor == preemptee) return;
    
    swap_event_workload(preemptor->excepts);
    switch_to(preemptee->thread_state, preemptor->thread_state);
}

Thread *get_curr_thread(){
    uint64_t *curr_state = get_curr_thread_state();
    return GET_CONTAINER(curr_state, Thread, thread_state);
}

/// @brief find the thread with target_id
/// @param location: return the pptr toward where the thread resides.
Thread *find_thread(int target_id, Queue **location){
    if(target_id == get_curr_thread()->id){
        *location = NULL;
        return get_curr_thread();
    }

    if(!queue_empty(&run_queue)){ // target in run queue?
        Thread *iter = GET_CONTAINER(queue_head(&run_queue), Thread, node);
        while(iter->id != target_id && iter->node.next){
            iter = GET_CONTAINER(iter->node.next, Thread, node); 
        }

        if(iter->id == target_id){
            *location = &run_queue;
            return iter;
        }
    }
    
    if(!queue_empty(&wait_queue)) { // target in wait queue?
        Thread *iter = GET_CONTAINER(queue_head(&wait_queue), Thread, node);
        while(iter->id != target_id && iter->node.next){
            iter = GET_CONTAINER(iter->node.next, Thread, node); 
        }

        if(iter->id == target_id) {
            *location = &wait_queue;
            return iter;
        }
    }
    
    return NULL;
}

/// @brief remove thread form current location and put it into `dead_queue`
/// @note the timer interrupt needed to be block during this process
void kill_curr_thread(){
    
    // critical section to ensure correctly remove from queue
    config_core_timer(false);
    Thread *curr = get_curr_thread();
    curr->alive = false;
    queue_push(&dead_queue, &curr->node);
    config_core_timer(true);
    
    schedule();
}

/// @brief remove thread form current's location and put it into dead_queue
void kill_thread(Thread *target, Queue *location){
    if(target == get_curr_thread()){
        kill_curr_thread(); // need critical section to ensure correctness
        return;
    }
    target->alive = false;
    queue_erase(location, &target->node);
    queue_push(&dead_queue, &target->node);
}

void curr_thread_regis_signal(int signal, SignalHandler handler_func){
    Thread *curr = get_curr_thread();
    curr->signal_handlers[signal] = handler_func;
    // init_sigal_handler(curr->signal_handlers[signal], handler_func);
}

// ----- local functions -----
void thread_init(Thread *target_thread, Task assigned, void *args, RCregion *prog){
    target_thread->id = total_thread++;
    target_thread->task = assigned;
    target_thread->priority = 0;
    target_thread->args = args;
    target_thread->prog = prog;
    target_thread->alive = true;

    target_thread->excepts = dyna_alloc(EXCEPT_WORKLOAD_SIZE);
    init_workload(target_thread->excepts);

    for(size_t i = 0; i < NUM_SIGNALS; i++){
        target_thread->signal_handlers[i] = NULL;
    }
    target_thread->last_signal = NUM_SIGNALS;

    for(size_t i = 0; i < 14; i++){
        switch(i){
        case STATE_LR:
            target_thread->thread_state[i] = (uint64_t)thread_entry;
            break;
        case STATE_USER_SP:
            target_thread->thread_state[i] = GET_STACK_BOTTOM((uint64_t)page_alloc(USER_STACK_SIZE), USER_STACK_SIZE);
            break;
        case STATE_KERNEL_SP:
            target_thread->thread_state[i] = GET_STACK_BOTTOM((uint64_t)page_alloc(KERNEL_STACK_SIZE), KERNEL_STACK_SIZE);
            break;
        default: // STATE_FP and others
            target_thread->thread_state[i] = 0;
        }
    }
}

bool thread_alive(Thread* thread){
    return thread->alive;
}

/// @brief The thread execution wrapper, all thread start from here when first scheduled
/// @param old_state state of previous thread, stored in register x0 by switch_to
/// @param new_state state of current thread
void thread_entry(void *old_state, uint64_t *new_state){
    Thread *curr = GET_CONTAINER(new_state, Thread, thread_state);
    curr->task(curr->args);
    kill_curr_thread(); // this line won't be executed if curr->task is launch_user_process()
} 

/// @brief assign this as a task to a thread when want to run user program as a new thread.
void launch_user_process(void *args){
    Thread *curr = get_curr_thread();
    _el1_to_el0(curr->prog->mem, (void *)curr->thread_state[STATE_USER_SP]);
    // program itself is responsible for calling sys_exit()
}

void switch_context(void *arg, uint64_t *trap_frame){
    add_timer_event(switch_interval, switch_context, NULL);
    // <- at this point, timer is unmasked by add_timer_event()
    schedule(); // currently fine due to long context switch period and exception priority.

    if(get_curr_thread()->last_signal != NUM_SIGNALS) handle_signal(trap_frame);
}

void enable_context_swtich(){
    add_timer_event(switch_interval, switch_context, NULL);
}

void idle(){
    while(true){
        kill_zombies();
        // schedule(); timer interrupt will call schedule instead
    }
}

void kill_zombies(){
    while(!queue_empty(&dead_queue)){
        ListNode *head = queue_pop(&dead_queue);
        Thread *dead_thread = GET_CONTAINER(head, Thread, node);
        dead_thread->thread_state[STATE_USER_SP] &= ~(USER_STACK_SIZE - 1);
        dead_thread->thread_state[STATE_KERNEL_SP] &= ~(KERNEL_STACK_SIZE - 1);
        
        rc_free(dead_thread->prog); // forked thread could still be running and need prog.
        page_free((void *)(dead_thread->thread_state[STATE_USER_SP]));
        page_free((void *)(dead_thread->thread_state[STATE_KERNEL_SP]));
        
        free_workload(dead_thread->excepts);

        // for(size_t i = 0; i < NUM_SIGNALS; i++){
        //     if(!dead_thread->signal_handlers[i]) continue;
        //     dyna_free(dead_thread->signal_handlers[i]);
        // }
        
        dyna_free((void *)dead_thread);
        
        char temp[20];
        send_string("[logger][Scheduler] kill pid ");
        send_line(itoa(dead_thread->id, temp, HEX));
    }
}

void __switch_thread(){
    asm volatile(
    ".global switch_to \n"
    "switch_to: \n"
        "stp x19, x20, [x0, 16 * 0] \n"
        "stp x21, x22, [x0, 16 * 1] \n"
        "stp x23, x24, [x0, 16 * 2] \n"
        "stp x25, x26, [x0, 16 * 3] \n"
        "stp x27, x28, [x0, 16 * 4] \n"
        "stp fp, lr, [x0, 16 * 5]   \n" // <- points to next line after switch_to()
        "mrs x9, sp_el0 \n"
        "mov x10, sp \n"                // sp_el1
        "stp x9, x10, [x0, 16 * 6]  \n"

        "ldp x19, x20, [x1, 16 * 0] \n"
        "ldp x21, x22, [x1, 16 * 1] \n"
        "ldp x23, x24, [x1, 16 * 2] \n"
        "ldp x25, x26, [x1, 16 * 3] \n"
        "ldp x27, x28, [x1, 16 * 4] \n"
        "ldp fp, lr, [x1, 16 * 5]   \n"
        "ldp x9, x10, [x1, 16 * 6]  \n"
        "mov sp, x10 \n"                // kernel sp
        "msr sp_el0, x9 \n"             // user sp
        "msr tpidr_el1, x1          \n"
        "ret \n"
    );
    asm volatile(
    ".global get_curr_thread_state \n"
    "get_curr_thread_state: \n"
        "mrs x0, tpidr_el1 \n"
        "ret \n"
    );
}