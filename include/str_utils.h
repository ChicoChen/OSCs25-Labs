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
size_t get_size(char *str);

//--- Conversion ---
char* itoa(unsigned int val, char *str, radix rad);
int atoi(char *str, radix rad);
uint32_t atoui(char *str, radix rad);
int carrtoi(char *str, unsigned int size, radix rad);
uint32_t ctoi(char c);

char to_lower(char c);

bool is_digit(char c);
bool is_hex_digit(char c);

//--- String Operation ---
int strcmp(char* str1, char* str2);
char* strrev(char* str);
char* strtok(char* str, char* terminators);
char *strchr(char *str, int c);
#endif