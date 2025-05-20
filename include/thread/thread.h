#ifndef THREAD_H
#define THREAD_H

#include "basic_type.h"
#include "exception/exception.h"
#include "allocator/rc_region.h"
#include "template/list.h"
#include "template/queue.h"
#include "thread/signals.h"

#define STATE_FP 10
#define STATE_LR 11
#define STATE_USER_SP 12
#define STATE_KERNEL_SP 13

#define GET_STACK_BOTTOM(head, size) head + size - 16
#define GET_STACK_TOP(ptr, size) ptr & ~(size - 1)

typedef void (*Task)(void *args);

#define THREAD_STATE_SIZE (8 * 14)
#define THREAD_STATE_OFFSET 32
#define THREAD_SIZE (56 + THREAD_STATE_SIZE + NUM_SIGNALS * 8 + LISTNODE_SIZE)

typedef struct{
    uint32_t id;
    uint32_t priority;

    RCregion *prog;
    Task task;
    void *args;
    uint64_t thread_state[14]; // need 16-byte aligned
    uint64_t *trap_frame;
    ExceptWorkload *excepts;
    // SignalHandler *signal_handlers[NUM_SIGNALS];
    SignalHandler signal_handlers[NUM_SIGNALS];
    int last_signal;
    bool alive;
    
    ListNode node;
}Thread;


void init_thread_sys();
Thread *make_thread(Task assigned_func, void *args, RCregion* prog);
void create_prog_thread(RCregion *prog_entry);
void schedule();

Thread *find_thread(int target_id, Queue **location);
void kill_thread(Thread *target, Queue *location);
void kill_curr_thread();

// void curr_thread_regis_signal(int signal, HandlerFunc handler_func);
void curr_thread_regis_signal(int signal, SignalHandler handler_func);

Thread *get_curr_thread();

#endif