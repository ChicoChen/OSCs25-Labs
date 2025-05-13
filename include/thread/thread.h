#ifndef THREAD_H
#define THREAD_H

#include "basic_type.h"

typedef void (*Task)(void *args);

void init_thread_sys();
void make_thread(Task assigned_func, void *args);
void create_prog_thread(void *prog_entry);
void schedule();

void thread_preempt(void *args);
void idle();
void exit();

uint32_t get_current_id();

#endif