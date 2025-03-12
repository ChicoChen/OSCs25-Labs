#ifndef STRING_UTILS_H
#define STRING_UTILS_H

typedef enum{
    BIN,
    DEC,
    HEX
} radix;

//--- Conversion ---
char* itoa(unsigned int val, char *str, radix rad);

//--- String Operation ---
int strcmp(char* str1, char* str2);
char* strrev(char* str);

#endif