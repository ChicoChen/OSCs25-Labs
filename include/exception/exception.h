#ifndef  EXCEPTION_H
#define EXCEPTION_H

#include "base_address.h"
#include "basic_type.h"

#define CORE0_INTERRUPT_SOURCE (volatile uint32_t *)(CORE_INTERRUPT_BASE + 0x60)

extern void* _exception_vector_table;

void init_exception();
void _el1_to_el0(addr_t return_addr, addr_t user_stack);

void _default_handler();
void curr_irq_handler();

void lower_sync_handler(void *stack_frame);

void print_el_message(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1);

#endif