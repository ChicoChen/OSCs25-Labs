#ifndef THREAD_H
#define THREAD_H

typedef void (*Task)(void *);

void init_thread_sys();
void *make_thread(Task assigned, uint32_t priority);
void schedule();

void preempt_callback(void *args);

#endif