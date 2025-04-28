#ifndef PAGE_FRAME_ALLOCATOR
#define PAGE_FRAME_ALLOCATOR

#include "basic_type.h"
#include "memory_region.h"

#define PAGE_SIZE 0x1000 // 4kB
#define TOTAL_PAGE (MEM_SIZE / PAGE_SIZE)

// #define PAGE_ALLOC_LOGGER
// #define PAGE_RESERVE_LOGGER

int init_page_array(void *start_addr);
void memory_reserve(void *start, size_t size);
void *page_alloc(size_t size);
void page_free(void *addr);

size_t to_block_idx(void *addr);
void *to_page_address(size_t block_idx);
bool is_allocated(void *addr);
#endif