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

//TODO: fix this
void atomic_add(addr_t address, int offset){
    // unsigned int status;
    // size_t old_value, new_value;
    // do {
    //     __asm__ volatile(
    //         "ldaxr %w0, [%1]"
    //         : "=r" (old_value)
    //         : "r" (address)
    //         : "memory"); //exclusive load
    //     new_value = old_value + offset;
    //     __asm__ volatile(
    //         "stlxr %w0, %w1, [%2]"
    //         : "=&r" (status)
    //         : "r" (new_value), "r" (address)
    //         : "memory"); //status 0 = success, 1 = failure
    // } while (status != 0);
}