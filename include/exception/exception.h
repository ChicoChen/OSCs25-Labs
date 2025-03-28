#ifndef  EXCEPTION_H
#define EXCEPTION_H

#include "../basic_type.h"

void _el1_to_el0(addr_t return_addr, addr_t user_stack);
void _set_exception_vec_table();
void _exception_handler();
void default_excep_entry();

#endif