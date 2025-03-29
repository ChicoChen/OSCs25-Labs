#ifndef N_UTILS_H
#define N_UTILS_H

#include "basic_type.h"

/* actually longer than given cycle due to loop delay*/
void delay(unsigned int cycle);
void addr_set(addr_t addr, uint32_t value);

uint32_t to_le_u32(uint32_t big_uint);

void atomic_add(addr_t address, int offset);

#endif