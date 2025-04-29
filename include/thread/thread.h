#ifndef THREAD_H
#define THREAD_H

#include "basic_type.h"

typedef void (*Task)();

void init_thread_sys();
void make_thread(Task assigned_func);
void schedule();

void thread_preempt(void *args);
void idle();
void exit();

uint32_t get_current_id();

#endif