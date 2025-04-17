#include "allocator/page_allocator.h"
#include "template/list.h"
#include "mini_uart.h"
#include "basic_type.h"
#include "utils.h"

typedef int PageStatus;
#define PAGE_GROUPED    -1
#define PAGE_OCCUPIED   -2
#define BLOCKSIZE_MAX_ORDER  8 // level 0~8

#define PAGEBLOCK_SIZE (8 + LISTNODE_SIZE)
typedef struct{
    PageStatus status;
    size_t idx;
    ListNode *node;
} PageBlock;

PageBlock *page_array = NULL;
PageBlock *free_list[BLOCKSIZE_MAX_ORDER + 1]; // record head of each length

// ----- Forward Declaration -----
#define GET_BUDDY(idx, level) (idx ^ (1 << level))

void init_block(PageBlock *block, PageStatus status, size_t idx);
void insert_free_list(PageBlock *new_block, PageStatus level);
void *to_page_address(PageBlock *block);
size_t to_block_idx(void *addr);

// ----- Public Interface -----
int init_page_array(void *start_addr){
    for(size_t i = 0; i <= 8; i++){
        free_list[i] = NULL;
    }

    start_addr = align(start_addr, 8);
    size_t total_pages = MAX_ADDRESS / PAGE_SIZE;
    page_array = (PageBlock *)start_addr;

    // initialize page array as minimum number of blocks
    for(size_t i = 0; i < total_pages; i++){
        PageStatus status = (i % (1 << BLOCKSIZE_MAX_ORDER))?
                            PAGE_GROUPED:
                            BLOCKSIZE_MAX_ORDER;
        init_block(page_array + i, status, i);
    }
}

void *page_alloc(size_t size){
    if(!size) return NULL;

    PageBlock *target = NULL;
    size_t order = calculate_order(size); // find minumum order.
    // find available free space
    for(size_t level = order; level <= BLOCKSIZE_MAX_ORDER; level++){
        if(!free_list[level]) continue;
        
        target = free_list[level];
        ListNode *next = list_remove(target->node);
        free_list[level] = (next)? GET_CONTAINER(next, PageBlock, node): NULL;
        break;
    }
    if(!target){
        _send_line_("[ERROR][page_allocator]: can't find a large enough space", async_send_data);
        return NULL;
    }
    
    // split if find a larger block
    while(target->status > order){
        size_t level = --target->status;
        size_t buddy_idx = GET_BUDDY(target->idx, level);
        page_array[buddy_idx].status = level;
        insert_free_list(page_array + buddy_idx, level);
    }

    target->status = PAGE_OCCUPIED;
    return to_page_address(target->idx);
}

// ----- Private Functions -----
void init_block(PageBlock *block, PageStatus status, size_t idx){
    block->status = status;
    block->idx = idx;
    list_init(block->node);
    insert_free_list(block, status);
}

void insert_free_list(PageBlock *new_block, PageStatus level){
    if(level < 0) return;
    ListNode *next = (free_list[level])? free_list[level]->node: NULL;
    list_add(new_block->node, NULL, next);
    free_list[level] = new_block;
}

void *to_page_address(PageBlock *block){
    return (void *)(block->idx * PAGE_SIZE);
}

size_t to_block_idx(void *addr){
    addr_t memaddr = (addr_t) addr;
    if(memaddr > MAX_ADDRESS) {
        _send_line_("[ERROR][page_allocator]: address to large, can't be translated", sync_send_data);
        return 0;
    }
    // else if(memaddr % PAGE_SIZE){
    //     _send_line_("[ERROR][page_allocator]: address need to be aligned to page size", sync_send_data);
    //     return 0;
    // }

    return (size_t) memaddr / PAGE_SIZE;
}

size_t calculate_order(size_t size){
    size = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    size--;
    size_t order = 0;
    while(size > 0){
        size = size >> 1;
        order++;
    }
    return order;
}