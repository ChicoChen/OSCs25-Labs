#ifndef TIMER_H
#define TIMER_H

#include "basic_type.h"

#define CORE0_TIMER_IRQ_CTRL (volatile unsigned int*)(0x40000040)

typedef struct{

} TimerEvent;

void core_timer_handler();
int config_core_timer(void *args);
void print_timeout_message();

#endif