#include "exception/exception.h"
#include "mini_uart.h"
#include "str_utils.h"

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