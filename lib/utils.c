#include "utils.h"
void delay(unsigned int cycle){
    for(int i = 0; i < cycle; i++){
        asm volatile("nop\n");
    }
}

void addr_set(addr_t addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

uint32_t to_le_u32(uint32_t big_uint){
    uint32_t little_uint = 0;
    for(int i = 0; i < 4; i++){
        little_uint = little_uint << 8;
        little_uint += big_uint &(0xFF);
        big_uint = big_uint >> 8;
    }
    return little_uint;
}