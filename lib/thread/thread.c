#include "thread/thread.h"
#include "allocator/dynamic_allocator.h"
#include "allocator/page_allocator.h"
#include "template/queue.h"
#include "timer/timer.h"
#include "memory_region.h"
#include "basic_type.h"
#include "utils.h"

#include "mini_uart.h"
#include "str_utils.h"

#define STATE_FP 10
#define STATE_LR 11
#define STATE_SP 12
#define STATE_DONE 13

#define THREAD_STACK_SIZE 0x1000

#define THREAD_STATE_SIZE (8 * 14)
#define THREAD_STATE_OFFSET 16
#define THREAD_SIZE (16 + THREAD_STATE_SIZE + LISTNODE_SIZE)
typedef struct{
    Task task;
    // void *args;
    uint32_t id;
    uint32_t priority;
    uint64_t thread_state[14]; // need 16-byte aligned
    // currently gurantee alignment because (slab header + prior data members)'s size are multiple of 16
    ListNode node;
}Thread;

static Queue run_queue;
static Queue wait_queue;
static Queue dead_queue;

static Thread *idle_thread = NULL;
static Thread *systemd = NULL;

static uint32_t total_thread = 0;
static uint64_t switch_interval = ~0;

// ----- forward decls -----

void thread_init(Thread *target_thrad, Task assigned);
Thread *get_curr_thread();
void task_wrapper(void *, uint64_t *state);
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

    idle_thread = (Thread *)kmalloc(THREAD_SIZE);
    thread_init(idle_thread, idle);
    
    systemd = (Thread *)kmalloc(THREAD_SIZE);
    thread_init(systemd, NULL);
    asm volatile("msr tpidr_el1, %[init_thread]" : :[init_thread]"r"(systemd) :);
}

void make_thread(Task assigned_func){
    Thread *new_thread = (Thread *)kmalloc(THREAD_SIZE);
    thread_init(new_thread, assigned_func);
    
    // insert into queue
    list_init(&new_thread->node);
    queue_push(&run_queue, &new_thread->node);
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

uint32_t get_current_id(){
    Thread *curr = get_curr_thread();
    return (curr)? curr->id: 0;
}

void thread_preempt(void *args){
    // todo: setup timer interrput for context switch
    // add_timer_event();

    schedule();
}


// ----- local functions -----

void thread_init(Thread *target_thrad, Task assigned){
    target_thrad->id = total_thread++;
    target_thrad->task = assigned;
    target_thrad->priority = 0;

    for(size_t i = 0; i < 14; i++){
        switch(i){
        case STATE_LR:
            target_thrad->thread_state[i] = (uint64_t)task_wrapper;
            break;
        case STATE_SP:
            target_thrad->thread_state[i] = (uint64_t)page_alloc(THREAD_STACK_SIZE) + THREAD_STACK_SIZE - 16;
            break;
        default: // STATE_FP and others
            target_thrad->thread_state[i] = 0;    
        }
    }
}

Thread *get_curr_thread(){
    uint64_t *curr_state = get_curr_thread_state();
    return GET_CONTAINER(curr_state, Thread, thread_state);
}

bool thread_alive(Thread* thread){
    return !(thread->thread_state[STATE_DONE]);
}

void task_wrapper(void *, uint64_t *state){
    Thread *curr = GET_CONTAINER(state, Thread, thread_state);
    curr->task();
    exit();
} // !lr will point to entry point of this func, exit need to resolve this

void exit(){
    Thread *curr_thread = get_curr_thread();
    curr_thread->thread_state[STATE_DONE] = true;
    queue_push(&dead_queue, &curr_thread->node);
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
        dead_thread->thread_state[STATE_SP] &= ~(THREAD_STACK_SIZE - 1);
        char temp[20];
        send_string("[logger][Scheduler] kill pid ");
        send_line(itoa(dead_thread->id, temp, HEX));
        page_free((void *)(dead_thread->thread_state[STATE_SP]));
        kfree((void *)dead_thread);
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
        "stp fp, lr, [x0, 16 * 5] \n" // <- points to next line after switch_to()
        "mov x9, sp \n"
        "str x9, [x0, 16 * 6] \n"

        "ldp x19, x20, [x1, 16 * 0] \n"
        "ldp x21, x22, [x1, 16 * 1] \n"
        "ldp x23, x24, [x1, 16 * 2] \n"
        "ldp x25, x26, [x1, 16 * 3] \n"
        "ldp x27, x28, [x1, 16 * 4] \n"
        "ldp fp, lr, [x1, 16 * 5] \n"
        "ldr x9, [x1, 16 * 6] \n"
        "mov sp,  x9 \n"
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