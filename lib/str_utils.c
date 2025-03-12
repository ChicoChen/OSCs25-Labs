#include "str_utils.h"

//--- Conversion ---
char* itoa(unsigned int val, char *str, radix rad){
    switch(rad){
    case HEX:
        str[0] = '0';   str[1] = 'x';
        int idx = 2;
        do{
            int digit = val % 16;
            val /= 16;
            if(digit < 10) str[idx++] = '0' + digit;
            else str[idx++] = 'a' + digit - 10;
        } while(val > 0);
        str[idx] = '\0';
        strrev(str + 2);
        break;
    default:
        str[0] = '\0';
    }

    return str;
}

//--- String Operation ---
int strcmp(char* str1, char* str2){
    int idx = 0;
    while(str1[idx] != '\0' && str2[idx] != '\0'){
        if(str1[idx] != str2[idx]){
            return 0;
        }
        idx++;
    }

    return str1[idx] == str2[idx];
}

char* strrev(char* str){
    int size = 0;
    while(str[size] != '\0') size++;

    for(int head = 0, tail = size - 1; head < tail;){
        char temp = str[head];
        str[head] = str[tail];
        str[tail] = temp;
        head++;
        tail--;
    }
    return str;
}