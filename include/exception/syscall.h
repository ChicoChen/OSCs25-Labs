#ifndef SYSCALL_H
#define SYSCALL_H

#include "basic_type.h"

void init_syscalls();
void invoke_syscall(uint64_t *stack_frame);

#endif