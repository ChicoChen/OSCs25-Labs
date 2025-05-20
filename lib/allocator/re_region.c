#include "allocator/rc_region.h"
#include "allocator/page_allocator.h"
#include "allocator/dynamic_allocator.h"

//todo: replace with kmalloc interface

RCregion *rc_alloc(size_t size){
    RCregion *region = (RCregion *)dyna_alloc(RC_REGION_SIZE);
    if(!region) return NULL;

    region->mem = page_alloc(size);
    if(!region->mem){
        dyna_free(region);
        return NULL;
    }

    region->count = 1;
    return region;
}

void add_ref(RCregion *region){
    region->count++;
}

void rc_free(RCregion *region){
    region->count--;
    if(!region->count) {
        page_free(region->mem);
        dyna_free(region);
    }
}
