#include "str_utils.h"

//--- Conversion ---
char* itoa(unsigned int val, char *str, radix rad){
    int idx;
    switch(rad){
    case HEX:
        str[0] = '0';   str[1] = 'x';
        idx = 2;
        do{
            int digit = val % 16;
            val /= 16;
            if(digit < 10) str[idx++] = '0' + digit;
            else str[idx++] = 'a' + digit - 10;
        } while(val > 0);
        str[idx] = '\0';
        strrev(str + 2);
        break;
    
    case DEC:
        str[0] = '0';
        idx = 0;
        do{
            str[idx++] = '0' + (val % 10);
            val /= 10;
        }while(val > 0);
        str[idx] = '\0';
        strrev(str);
        break;

    default:
        str[0] = '\0';
    }

    return str;
}

int atoi(char *str, radix rad){
    int num = 0;
    int idx = 0;
    switch(rad){
    case DEC:
        while(str[idx] != '\0'){
            if(str[idx] > '9' || str[idx] < '0') return -1;
            num *= 10;
            num += str[idx++] - '0';
            
        }
        break;

    default:
        num = -1;
        break;
    }

    return num;
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