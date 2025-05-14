#ifndef THREAD_H
#define THREAD_H

#include "basic_type.h"
#include "template/list.h"
#include "allocator/rc_region.h"

#define STATE_FP 10
#define STATE_LR 11
#define STATE_USER_SP 12
#define STATE_KERNEL_SP 13

#define THREAD_STATE_SIZE (8 * 14)
#define THREAD_STATE_OFFSET 32
#define THREAD_SIZE (36 + THREAD_STATE_SIZE + LISTNODE_SIZE)

#define GET_STACK_BOTTOM(head, size) head + size - 16
#define GET_STACK_TOP(ptr, size) ptr & ~(size - 1)

typedef void (*Task)(void *args);

typedef struct{
    uint32_t id;
    uint32_t priority;
    RCregion *prog;
    Task task;
    void *args;
    uint64_t thread_state[14]; // need 16-byte aligned
    bool alive;
    ListNode node;
}Thread;


void init_thread_sys();
Thread *make_thread(Task assigned_func, void *args, RCregion* prog);
void create_prog_thread(RCregion *prog_entry);
void schedule();

void thread_preempt(void *args);
void idle();
void terminate_thread();
void kill_thread(int id);

Thread *get_curr_thread();

#endif