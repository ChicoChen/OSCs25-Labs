#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "basic_type.h"

typedef enum{
    BIN,
    DEC,
    HEX
} radix;

//--- Creations --;
char *make_str(char *str, char c, size_t len);

//--- Getters ---
unsigned int get_size(char *str);

//--- Conversion ---
char* itoa(unsigned int val, char *str, radix rad);
int atoi(char *str, radix rad);
int carrtoi(char *str, unsigned int size, radix rad);

//--- String Operation ---
int strcmp(char* str1, char* str2);
char* strrev(char* str);

#endif