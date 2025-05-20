#ifndef  EXCEPTION_H
#define EXCEPTION_H

#include "base_address.h"
#include "basic_type.h"

#define CORE0_INTERRUPT_SOURCE (volatile uint32_t *)(CORE_INTERRUPT_BASE + 0x60)

#define ENABLE_DAIF asm volatile("msr DAIFClr, 0xf")
#define DISABLE_DAIF asm volatile("msr DAIFSet, 0xf")

#define EXCEPT_WORKLOAD_SIZE 16
typedef struct{
    struct ExceptQueue *base_queue;
    bool timer_mask;
    bool uart_mask;
}ExceptWorkload;

extern void* _exception_vector_table;

void init_exception();
void _el1_to_el0(void *return_addr, void *user_stack);

void _default_handler();
void irq_handler(uint64_t *trap_frame);
void lower_sync_handler(uint64_t *trap_frame);

void init_workload(ExceptWorkload *workload);
void free_workload(ExceptWorkload *workload);
ExceptWorkload *get_curr_workload();
void swap_event_workload(ExceptWorkload *new_workload);

void print_el_message(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1);

#endif