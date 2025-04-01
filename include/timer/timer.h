#ifndef TIMER_H
#define TIMER_H

#include "basic_type.h"

#define CORE0_TIMER_IRQ_CTRL (volatile unsigned int*)(0x40000040)
#define TIMER_INTERRUPT_MIN_INTERVAL 10
#define TIMEREVENT_BYTESIZE 32

typedef struct TimerEvent{
    uint64_t target_clock;
    void (*callback_func)(void* args);
    void *args;
    struct TimerEvent *next;
} TimerEvent;

typedef struct{
    TimerEvent *head;
    bool initialized;
} EventQueue;

void enable_core_timer(bool enable);

void init_core_timer();
void timer_interrupt_handler();
int add_event(uint64_t offset, void (*callback_func)(void* arg), void *args);
void timer_clear_event(void (*callback_func)(void* arg));

void tick_callback(void* args);
void print_tick_message();
#endif