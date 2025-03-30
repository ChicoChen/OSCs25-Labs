#ifndef TIMER_H
#define TIMER_H

#include "basic_type.h"

#define CORE0_TIMER_IRQ_CTRL (volatile unsigned int*)(0x40000040)
#define TIMER_INTERRUPT_MIN_INTERVAL 10
#define TIMEREVENT_BYTESIZE 32

typedef struct TimerEvent{
    uint64_t target_clock;
    int (*callback_func)(void* args);
    void *args;
    struct TimerEvent *next;
} TimerEvent;

typedef struct{
    TimerEvent *head;
    bool initialized;
} EventQueue;

void init_core_timer();
void core_timer_handler();
int add_event(uint64_t offset, int (*callback_func)(void* arg), void *args);
int config_core_timer(void *args);

int tick_callback(void* args);
void print_tick_message();

// ----- Private Members -----
static inline void enable_core_timer(bool enable){
    asm volatile("msr cntp_ctl_el0, %[flag]"::[flag]"r"(enable):); // don't affect cntpct_el0
}

static inline void get_timer(uint64_t *count, uint64_t *freq){
    asm volatile(
        "mrs %[count], cntpct_el0\n"
        "mrs %[freq], cntfrq_el0\n"
        : [count]"=r"(*count), [freq]"=r"(*freq)::);
    }

static void set_timeout(uint64_t target_cval);

static EventQueue events;
    
#endif