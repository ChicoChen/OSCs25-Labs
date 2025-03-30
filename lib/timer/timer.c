#include "timer/timer.h"
#include "mini_uart.h"
#include "str_utils.h"

void core_timer_handler(){
    uint64_t count;
    uint64_t freq;

    asm volatile(
        "mrs %[count], cntpct_el0\n"
        "mrs %[freq], cntfrq_el0\n"
        :[count]"=r"(count), [freq]"=r"(freq)
        ::);
    print_timeout_message(count, freq);

    //set timer interrupt
    uint64_t offset = freq * 2;
    asm volatile(
        "msr cntp_tval_el0, %[offset]\n"
        :
        : [offset]"r"(offset)
        :
    );
}

int config_core_timer(void *args){
    char *flag = *(char **)args;
    if(!flag) return 0;
    bool enable = BOOL(atoi(flag, DEC));
    asm volatile("msr cntp_ctl_el0, %[flag]"::[flag]"r"(enable):); // enable or disable timer interrupt

    uint64_t freq;
    asm volatile("mrs %[freq], cntfrq_el0": [freq]"=r"(freq)::);

    uint32_t offset = 2 * (uint32_t)freq;
    asm volatile("msr cntp_tval_el0, %[offset]"::[offset]"r"(offset):);
    *CORE0_TIMER_IRQ_CTRL = 2; //unmask core0 timer interrupt
    
    return 1;
}

void print_timeout_message(uint64_t count, uint64_t freq){
    unsigned int second = count / freq;
    _send_line_("", sync_send_data);
    sync_send_data('[');
    char sec_str[10];
    _send_string_(itoa(second, sec_str, DEC), sync_send_data);
    _send_line_("]", sync_send_data);
}