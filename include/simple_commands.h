#ifndef SIMPLE_COMMANDS_H
#define SIMPLE_COMMANDS_H

#include "base_address.h"

#define CMD_BUFFER_SIZE 256

#define PM_RSTC     (MMIO_BASE + 0x10001c)
#define PM_WDOG     (MMIO_BASE + 0x100024)

#define PM_PASSWORD 0x5a000000

typedef struct{
    char *name;
    int (*func)(void*);
    char* description;
} Command;

extern Command commands[];

int cmd_help(void* args);
int hello_world(void* args);
int reset(void *arg);
int dts_wrapper(void *arg);

#endif