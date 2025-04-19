#ifndef N_UTILS_H
#define N_UTILS_H

#include "basic_type.h"

/* actually longer than given cycle due to loop delay*/
void delay(unsigned int cycle);

// memory and address manipulation
void *memcpy(void *str1, void *str2, size_t size);
void *align(void *addr, size_t base);
void addr_set(addr_t addr, uint32_t value);

// math operation
uint32_t to_le_u32(uint32_t big_uint);
size_t roundup_pow2(size_t num);

void atomic_add(addr_t address, int offset);

#endif