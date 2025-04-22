#ifndef SIMPLE_COMMANDS_H
#define SIMPLE_COMMANDS_H

#include "base_address.h"

#define PM_RSTC     (MMIO_BASE + 0x10001c)
#define PM_WDOG     (MMIO_BASE + 0x100024)

#define PM_PASSWORD 0x5a000000

typedef struct{
    char *name;
    int (*func)(void*);
    char* description;
} Command;

extern Command commands[];

#endif