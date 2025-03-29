#include "exception/exception.h"
#include "str_utils.h"

void print_el_message(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1){
    char reg_content[20];
    send_string("spsr_el1: ");
    send_line(itoa(spsr_el1, reg_content, HEX));
    
    send_string("elr_el1: ");
    send_line(itoa(elr_el1, reg_content, HEX));
    
    send_string("esr_el1: ");
    send_line(itoa(esr_el1, reg_content, HEX));
    
    return;
}

void curr_irq_handler(){
    uint32_t source = *CORE0_INTERRUPT_SOURCE;
    if(source & 0x01u << 1) core_timer_handler();
    else if(source & 0x01u << 8) uart_except_handler();
    else{
        send_line("[unknown irq interrupt]"); // use sychronous
    }
}

// ----- timer -----
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
    _set_core_timer(enable);
    return 1;
}

void print_timeout_message(uint64_t count, uint64_t freq){
    unsigned int second = count / freq;
    send_line("");
    send_data('[');
    char sec_str[10];
    send_string(itoa(second, sec_str, DEC));
    send_line("]");
}