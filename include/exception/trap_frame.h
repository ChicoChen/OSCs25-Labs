#ifndef TRAP_FRAME_H
#define TRAP_FRAME_H

#define SVC 0b010101

#define TRAP_FRAME_SIZE 272
#define TRAP_FRAME_NUM_ELEMENTS 34

#define SYSCALL_IDX     8
#define LR_IDX          30
#define SPSR_IDX        31
#define ELR_IDX         32
#define ESR_IDX         33

#endif