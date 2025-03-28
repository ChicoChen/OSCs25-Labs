#ifndef  EXCEPTION_H
#define EXCEPTION_H

#include "../basic_type.h"

int config_core_timer(void *args);
void default_excep_entry();
void print_timeout_message();

void _el1_to_el0(addr_t return_addr, addr_t user_stack);
void _init_exception();
void _exception_handler();

void _set_core_timer(bool enable);
void _core_timer_handler();

#endif