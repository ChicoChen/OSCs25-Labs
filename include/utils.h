#ifndef N_UTILS_H
#define N_UTILS_H

#include "basic_type.h"

#define GET_OFFSET(type, member) ((uint64_t) &((type *)0)->member)
#define GET_CONTAINER(ptr, type, member) ((type *)((addr_t)ptr - GET_OFFSET(type, member)))

#define COND_SWAP(flag, type, var1, var2)   \
    do{                                     \
        if (flag) {                         \
            type temp = var1;               \
            var1 = var2;                    \
            var2 = temp;                    \
        }                                   \
    }while(0)

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