#include "utils.h"
void delay(unsigned int cycle){
    for(int i = 0; i < cycle; i++){
        asm volatile("nop\n");
    }
}