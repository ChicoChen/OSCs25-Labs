#ifndef DYNAMIC_ALLOCATOR_H
#define DYNAMIC_ALLOCATOR_H

#include "basic_type.h"

void init_memory_pool();

void *kmalloc(size_t size);
void kfree(void *target);

#endif