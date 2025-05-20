#ifndef TIMER_H
#define TIMER_H

#include "basic_type.h"

#define CORE0_TIMER_IRQ_CTRL (volatile unsigned int*)(0x40000040)
#define TIMER_INTERRUPT_MIN_INTERVAL 10
#define TIMEREVENT_BYTESIZE 32

typedef struct TimerEvent{
    uint64_t target_clock;
    void (*callback_func)(void* args, uint64_t *trap_frame);
    void *args;
    struct TimerEvent *next;
} TimerEvent;

typedef struct{
    TimerEvent *head;
    bool initialized;
} EventQueue;

void init_core_timer();
void config_core_timer(bool enable);
void get_timer(uint64_t *count, uint64_t *freq);

void timer_interrupt_handler(uint64_t *trap_frame);
int add_timer_event(uint64_t offset, void (*callback_func)(void* arg, uint64_t *trap_frame), void *args);
void clear_timer_event(void (*callback_func)(void* arg, uint64_t *trap_frame));

void tick_callback(void* args, uint64_t *trap_frame);
#endif