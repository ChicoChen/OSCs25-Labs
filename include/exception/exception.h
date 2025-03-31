#ifndef  EXCEPTION_H
#define EXCEPTION_H

#include "../base_address.h"
#include "../basic_type.h"

#define CORE0_INTERRUPT_SOURCE (volatile uint32_t *)(CORE_INTERRUPT_BASE + 0x60)
#define IRQ_TASK_SIZE 32

extern void* _exception_vector_table;

typedef enum {
// lower
    UART,
    TIMER
// higher
}IrqPriority;

// TODO: a standard link-list template (please refer to linux approach)
typedef struct IrqTask{
    struct IrqTask *prev;
    struct IrqTask *next;
    void (*handler)();
    IrqPriority priority;
}IrqTask;

extern void timer_interrupt_handler(); //"timer/timer.h"

void init_exception();
void _el1_to_el0(addr_t return_addr, addr_t user_stack);

void _default_handler();
void curr_irq_handler();

void print_el_message(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1);

// ----- static -----
static IrqTask* task_head;


#endif