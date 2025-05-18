#include "timer/timer.h"
#include "exception/exception.h"
#include "allocator/dynamic_allocator.h"
#include "mini_uart.h"
#include "str_utils.h"

static EventQueue events;
// ----- forward decls ------

void print_tick_message();
static void set_timeout(uint64_t target_cval);
void enable_el0_timer();

// ----- public interfaces ------

void config_core_timer(bool enable){
    asm volatile("msr cntp_ctl_el0, %[flag]"::[flag]"r"(enable):);
    get_curr_workload()->timer_mask = BOOL(!enable);
}

void init_core_timer(){
    events.head = NULL;
    events.initialized = true;
    config_core_timer(false);
    *CORE0_TIMER_IRQ_CTRL = 2; //unmask core0 timer interrupt
    enable_el0_timer();
}

void timer_interrupt_handler(){
    uint64_t current_count;
    uint64_t freq;
    get_timer(&current_count, &freq);
    
    // trigger all due timeout event
    while(events.head){
        if(events.head->target_clock > current_count) break;

        TimerEvent *temp = events.head;
        events.head = events.head->next;
        temp->callback_func(temp->args); // !scheduler() wont return from here upon switch
        dyna_free((void *)temp);
    }

    if(events.head) {
        set_timeout(events.head->target_clock);
        config_core_timer(true);
    }
}

int add_timer_event(uint64_t offset, void (*callback_func)(void *arg), void *args){
    if(!events.initialized){
        _send_line_("[timer] timer not init!", sync_send_data);
        return 1;
    }
    
    uint64_t current_clock, freq;
    get_timer(&current_clock, &freq);
    
    TimerEvent *new_event = (TimerEvent *)dyna_alloc(TIMEREVENT_BYTESIZE);
    if(!new_event) {
        _send_line_("[timer] can't alloc timer event", sync_send_data);
        return 1;
    }
    
    new_event->callback_func = callback_func;
    new_event->args = args;
    new_event->target_clock = current_clock + offset;
    
    //critical section
    config_core_timer(false);
    TimerEvent *iter = events.head;
    TimerEvent *prev = NULL;
    while(iter != NULL){
        if(iter->target_clock > new_event->target_clock) break;
        prev = iter;
        iter = iter->next;
    }
    new_event->next = iter;
    if(prev) prev->next = new_event;
    else {
        events.head = new_event;
        set_timeout(new_event->target_clock);
    }
    
    config_core_timer(true);
    return 0;
}

void clear_timer_event(void (*callback_func)(void *arg)){
    config_core_timer(false);
    while(events.head){
        if(events.head->callback_func == tick_callback) events.head = events.head -> next;
        else break;
    }
    if(!events.head) return;

    TimerEvent *iter = events.head;
    while(iter->next){
        if(iter->next->callback_func == tick_callback) iter->next = iter->next->next;
        else iter = iter->next;
    }
    config_core_timer(true);
}

void tick_callback(void *args){
    print_tick_message();
    uint64_t current_count;
    uint64_t freq;
    get_timer(&current_count, &freq);
    add_timer_event(2 * freq, tick_callback, args);
    return;
}

void get_timer(uint64_t *count, uint64_t *freq){
    asm volatile(
        "mrs %[count], cntpct_el0\n"
        "mrs %[freq], cntfrq_el0\n"
        : [count]"=r"(*count), [freq]"=r"(*freq)::
    );
}

// ----- local functions ------

static void set_timeout(uint64_t target_cval){
    uint64_t current, freq;
    get_timer(&current, &freq);
    if(current + TIMER_INTERRUPT_MIN_INTERVAL >= target_cval) target_cval = current + TIMER_INTERRUPT_MIN_INTERVAL;
    asm volatile("msr cntp_cval_el0, %[clock]":: [clock]"r"(target_cval):);
}

void print_tick_message(){
    uint64_t current_count, freq;
    get_timer(&current_count, &freq);
    unsigned int second = current_count / freq;
    _send_line_("", sync_send_data);
    sync_send_data('[');
    char sec_str[10];
    _send_string_(itoa(second, sec_str, DEC), sync_send_data);
    _send_line_("]", sync_send_data);
}

void enable_el0_timer(){
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}