#ifndef PAGE_FRAME_ALLOCATOR
#define PAGE_FRAME_ALLOCATOR

#include "basic_type.h"
#include "base_address.h"

#define PAGE_SIZE 0x1000 // 4kB
#define PAGE_ARRAY_START 0x10000000
#define PAGE_ARRAY_END 0x20000000

int init_page_array(void *start_addr);


#endif