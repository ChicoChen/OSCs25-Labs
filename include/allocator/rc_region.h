#ifndef REF_COUNT_REGION_H
#define REF_COUNT_REGION_H

#include "basic_type.h"

#define RC_REGION_SIZE 12
typedef struct{
    size_t count;
    void *mem;
} RCregion;

RCregion *rc_alloc(size_t size);
void add_ref(RCregion *region);
void rc_free(RCregion *region);

#endif