#include "timer/timer.h"
#include "mini_uart.h"
#include "str_utils.h"
#include "allocator/simple_alloc.h"

void init_core_timer(){
    events.head = NULL;
    events.initialized = true;
    enable_core_timer(false);
    *CORE0_TIMER_IRQ_CTRL = 2; //unmask core0 timer interrupt
}

void timer_interrupt_handler(){
    uint64_t current_count;
    uint64_t freq;
    get_timer(&current_count, &freq);
    
    TimerEvent* iter = events.head;
    while(iter){
        if(iter->target_clock > current_count) break;

        // _send_line_("trigger callback", sync_send_data);
        iter->callback_func(iter->args);
        
        iter = iter->next;
    }

    events.head = iter;
    if(iter) set_timeout(iter->target_clock);
    else enable_core_timer(false);
}

int add_event(uint64_t offset, void (*callback_func)(void* arg), void *args){
    // _send_line_("call add_event", sync_send_data);
    if(!events.initialized){
        _send_line_("[timer not init!]", sync_send_data);
        return 1;
    }
    
    //! TODO: currently no deallocator
    uint64_t current_clock, freq;
    get_timer(&current_clock, &freq);
    
    TimerEvent *new_event = (TimerEvent *)simple_alloc(TIMEREVENT_BYTESIZE);
    if(!new_event) return 1;
    
    new_event->callback_func = callback_func;
    new_event->args = args;
    new_event->target_clock = current_clock + offset * freq;
    
    enable_core_timer(false);
    //critical section
    TimerEvent *iter = events.head;
    TimerEvent* prev = NULL;
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
    
    enable_core_timer(true);
    return 0;
}

void timer_clear_event(void (*callback_func)(void* arg)){
    enable_core_timer(false);
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
    enable_core_timer(true);
}

void tick_callback(void* args){
    print_tick_message();
    add_event(2, tick_callback, args);
    return;
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

void set_timeout(uint64_t target_cval){
    uint64_t current, freq;
    get_timer(&current, &freq);
    if(current + TIMER_INTERRUPT_MIN_INTERVAL >= target_cval) target_cval = current + TIMER_INTERRUPT_MIN_INTERVAL;
    asm volatile("msr cntp_cval_el0, %[clock]":: [clock]"r"(target_cval):);
}
