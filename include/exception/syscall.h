#ifndef SYSCALL_H
#define SYSCALL_H

#include "basic_type.h"

#define SYSCALL_IDX     8
#define SPSR_IDX        31
#define ELR_IDX         32
#define ESR_IDX         33

#define SVC 0b010101

void init_syscalls();
void invoke_syscall(uint64_t *stack_frame);

#endif