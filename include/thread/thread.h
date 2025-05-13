#ifndef THREAD_H
#define THREAD_H

#include "basic_type.h"
#include "template/list.h"

#define STATE_FP 10
#define STATE_LR 11
#define STATE_USER_SP 12
#define STATE_KERNEL_SP 13

#define THREAD_STATE_SIZE (8 * 14)
#define THREAD_STATE_OFFSET 16
#define THREAD_SIZE (36 + THREAD_STATE_SIZE + LISTNODE_SIZE)

typedef void (*Task)(void *args);

typedef struct{
    void *prog;
    uint32_t id;
    uint32_t priority;
    uint64_t thread_state[14]; // need 16-byte aligned
    Task task;
    void *args;
    bool alive;
    ListNode node;
}Thread;


void init_thread_sys();
void make_thread(Task assigned_func, void *args, void* prog);
void create_prog_thread(void *prog_entry);
void schedule();

void thread_preempt(void *args);
void idle();
void exit();

Thread *get_curr_thread();

#endif