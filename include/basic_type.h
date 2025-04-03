#ifndef BASIC_TYPE_H
#define BASIC_TYPE_H

#define NULL 0

typedef enum { false = 0, true = 1 } bool;
#define BOOL(x) ((x) ? 1 : 0)

typedef char byte;

typedef unsigned int uint32_t;
typedef unsigned int size_t;

typedef unsigned long long uint64_t;
typedef unsigned long long addr_t;
#endif