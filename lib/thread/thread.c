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
void task_wrapper(void *, uint64_t *state);
void launch_user_process(void *prog_entry);
bool thread_alive(Thread* thread);
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
    asm volatile("msr tpidr_el1, %[init_thread]" : :[init_thread]"r"(systemd->thread_state) :);
}

void make_thread(Task assigned_func, void *args, RCregion* prog){
    Thread *new_thread = (Thread *)dyna_alloc(THREAD_SIZE);
    thread_init(new_thread, assigned_func, args, prog);
    
    // insert into queue
    node_init(&new_thread->node);
    queue_push(&run_queue, &new_thread->node);
}

void create_prog_thread(RCregion *prog_entry){
    // 1. create thread
    // 2. jump to el0, notice return address
    // 3. schedule to execute it 
    // 4. todo: leave memory reclaiming to idle
    make_thread(launch_user_process, NULL, prog_entry);
}

void schedule(){
    // get current thread and next thread
    Thread *preemptee = get_curr_thread();
    if(preemptee->id > 1 && thread_alive(preemptee)){
        queue_push(&run_queue, &preemptee->node);
    }   
    
    ListNode* head = queue_pop(&run_queue);
    Thread *preemptor = (!head)? idle_thread: GET_CONTAINER(head, Thread, node);
    if(preemptor == preemptee) return;
    
    // save current state and context switch
    // get preempt during this call, will resume from here
    switch_to(preemptee->thread_state, preemptor->thread_state);
}

Thread *get_curr_thread(){
    uint64_t *curr_state = get_curr_thread_state();
    return GET_CONTAINER(curr_state, Thread, thread_state);
}

// ----- local functions -----
void thread_init(Thread *target_thread, Task assigned, void *args, RCregion *prog){
    target_thread->id = total_thread++;
    target_thread->task = assigned;
    target_thread->priority = 0;
    target_thread->args = args;
    target_thread->prog = prog;
    target_thread->alive = true;

    for(size_t i = 0; i < 14; i++){
        switch(i){
        case STATE_LR:
            target_thread->thread_state[i] = (uint64_t)task_wrapper;
            break;
        case STATE_USER_SP:
            target_thread->thread_state[i] = (uint64_t)page_alloc(USER_STACK_SIZE) + USER_STACK_SIZE - 16;
            break;
        case STATE_KERNEL_SP:
            target_thread->thread_state[i] = (uint64_t)page_alloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE - 16;
            break;
        default: // STATE_FP and others
            target_thread->thread_state[i] = 0;
        }
    }
}


bool thread_alive(Thread* thread){
    return !(thread->alive);
}

void task_wrapper(void *old_state, uint64_t *new_state){
    Thread *curr = GET_CONTAINER(new_state, Thread, thread_state);
    curr->task(curr->args);
    exit(); //todo: replace with sys_call_exit()
} 

void launch_user_process(void *args){
    Thread *curr = get_curr_thread();
    _el1_to_el0(curr->prog->mem, (void *)curr->thread_state[STATE_USER_SP]);
    // program itself is responsible for calling sys_exit()
}

void exit(){
    Thread *curr_thread = get_curr_thread();
    curr_thread->alive = false;
    queue_push(&dead_queue, &curr_thread->node);
    //! exit() could run in el0, cause schedule() die cause try to access el1 register
    schedule();
}

void idle(){
    while(true){
        kill_zombies();
        schedule();
    }
}

void kill_zombies(){
    while(!queue_empty(&dead_queue)){
        ListNode *head = queue_pop(&dead_queue);
        Thread *dead_thread = GET_CONTAINER(head, Thread, node);
        dead_thread->thread_state[STATE_USER_SP] &= ~(USER_STACK_SIZE - 1);
        dead_thread->thread_state[STATE_KERNEL_SP] &= ~(KERNEL_STACK_SIZE - 1);
        
        char temp[20];
        send_string("[logger][Scheduler] kill pid ");
        send_line(itoa(dead_thread->id, temp, HEX));

        page_free((void *)(dead_thread->prog));
        page_free((void *)(dead_thread->thread_state[STATE_USER_SP]));
        page_free((void *)(dead_thread->thread_state[STATE_KERNEL_SP]));
        dyna_free((void *)dead_thread);
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
        "mov sp, x10 \n"
        "msr sp_el0, x9 \n"
        "msr tpidr_el1, x1 \n"
        "ret \n"
    );
    asm volatile(
    ".global get_curr_thread_state \n"
    "get_curr_thread_state: \n"
        "mrs x0, tpidr_el1 \n"
        "ret \n"
    );
}