#include "exception/exception.h"
#include "mini_uart.h"
#include "str_utils.h"

int config_core_timer(void *args){
    char *flag = *(char **)args;
    bool enable = BOOL(atoi(flag, DEC));
    _set_core_timer(enable);
    return 1;
}

void default_excep_entry(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1){
    char reg_content[20];
    send_string("spsr_el1: ");
    send_line(itoa(spsr_el1, reg_content, HEX));

    send_string("elr_el1: ");
    send_line(itoa(elr_el1, reg_content, HEX));

    send_string("esr_el1: ");
    send_line(itoa(esr_el1, reg_content, HEX));

    return;
}

void print_timeout_message(uint64_t count, uint64_t freq){
    unsigned int second = count / freq;
    send_line("");
    send_data('[');
    char sec_str[10];
    send_string(itoa(second, sec_str, DEC));
    send_line("]");
}