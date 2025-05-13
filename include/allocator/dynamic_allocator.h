#ifndef DYNAMIC_ALLOCATOR_H
#define DYNAMIC_ALLOCATOR_H

#include "basic_type.h"

// #define DYNAMIC_ALLOC_LOGGER

void init_memory_pool();

void *dyna_alloc(size_t size);
void dyna_free(void *target);

#endif