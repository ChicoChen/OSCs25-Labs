#ifndef SIGNALS_H
#define SIGNALS_H

#include "basic_type.h"
#include "exception/trap_frame.h"

#define NUM_SIGNALS 10
#define SIGNAL_HANDLER_STACK_SIZE 0x1000

typedef void (*SignalHandler)();
void handle_signal(uint64_t *trap_frame);

#endif