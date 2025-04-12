#include "allocator/page_allocator.h"
#include "template/list.h"
#include "mini_uart.h"
#include "basic_type.h"
#include "utils.h"

typedef int PageStatus;
#define PAGE_GROUPED    -1
#define PAGE_OCCUPIED   -2
#define BLOCKSIZE_MAX_ORDER  8 // level 0~8

#define PAGEBLOCK_SIZE      (8 + LISTNODE_SIZE)
typedef struct{
    PageStatus status;
    size_t idx;
    ListNode *node;
} PageBlock;

PageBlock *page_array = NULL;
PageBlock *free_list[BLOCKSIZE_MAX_ORDER + 1]; // record head of each list

// ----- Forward Declaration -----
void init_block(PageBlock *block, PageStatus status, size_t i);
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

    // initialize page array as minimum number blocks
    for(size_t i = 0; i < total_pages; i++){
        PageStatus status = (i % (1 << BLOCKSIZE_MAX_ORDER))?
                            PAGE_GROUPED:
                            BLOCKSIZE_MAX_ORDER;
        init_block(page_array + i, PAGE_GROUPED, i);
    }
}

// ----- Private Functions -----
void init_block(PageBlock *block, PageStatus status, size_t i){
    block->status = status;
    block->idx = i;
    list_init(block->node);
    insert_free_list(block, status);
}

void insert_free_list(PageBlock *new_block, PageStatus level){
    if(level < 0) return;
    list_add(new_block, NULL, free_list[level]);
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
    else if(memaddr % PAGE_SIZE){
        _send_line_("[ERROR][page_allocator]: address need to be aligned to page size", sync_send_data);
        return 0;
    }

    return (size_t) memaddr / PAGE_SIZE;
}