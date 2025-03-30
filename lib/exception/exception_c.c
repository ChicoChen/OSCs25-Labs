#include "exception/exception.h"
#include "mini_uart.h"
#include "str_utils.h"

void print_el_message(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1){
    char reg_content[20];
    _send_string_("spsr_el1: ", sync_send_data);
    _send_line_(itoa(spsr_el1, reg_content, HEX), sync_send_data);
    
    _send_string_("elr_el1: ", sync_send_data);
    _send_line_(itoa(elr_el1, reg_content, HEX), sync_send_data);
    
    _send_string_("esr_el1: ", sync_send_data);
    _send_line_(itoa(esr_el1, reg_content, HEX), sync_send_data);
    
    return;
}

void curr_irq_handler(){
    uint32_t source = *CORE0_INTERRUPT_SOURCE;
    if(source & 0x01u << 1) core_timer_handler();
    else if(source & 0x01u << 8) uart_except_handler();
    else{
        _send_line_("[unknown irq interrupt]", sync_send_data);
    }
}