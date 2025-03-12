#include "utils.h"
void delay(unsigned int cycle){
    for(int i = 0; i < cycle; i++){
        asm volatile("nop\n");
    }
}

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}