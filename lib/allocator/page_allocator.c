#include "allocator/page_allocator.h"
#include "template/list.h"
#include "memory_region.h"
#include "basic_type.h"
#include "mini_uart.h"
#include "utils.h"
#include "str_utils.h"

typedef int PageStatus;
#define PAGE_GROUPED    -1
#define PAGE_OCCUPIED   -2
#define PAGE_RESERVED   -3
#define BLOCK_MAX_ORDER  8 // level 0~8

#define PAGEBLOCK_SIZE (8 + LISTNODE_SIZE)
size_t page_array_size = TOTAL_PAGE * PAGEBLOCK_SIZE;
typedef struct{
    PageStatus status;
    size_t idx;
    ListNode node;
} PageBlock;

PageBlock *page_array = NULL;
PageBlock *free_list[BLOCK_MAX_ORDER + 1]; // record head of each length

#if defined(PAGE_ALLOC_LOGGER) || defined(PAGE_RESERVE_LOGGER)
char pagelog_buffer[128];
#endif

// ----- Forward Declaration -----
#define GET_BUDDY(idx, level)               (idx ^ (1 << level))
#define BUDDY_MERGEABLE(buddy_idx, level)   (page_array[buddy_idx].status == level || page_array[buddy_idx].status == PAGE_GROUPED)

void init_block(PageBlock *block, PageStatus status, size_t idx);
size_t merge_page_block(size_t target_idx, size_t buddy_idx, size_t level);

void free_list_insert(PageBlock *new_block, PageStatus level);
PageBlock *free_list_pop(size_t level);
void free_list_remove(size_t idx, size_t level);

size_t calculate_order(size_t size);
size_t segment_block_seq(size_t start_idx, size_t size);

void *to_page_address(PageBlock *block);

// ----- Public Interface -----
int init_page_array(void *start_addr){
    for(size_t i = 0; i <= 8; i++){
        free_list[i] = NULL;
    }

    start_addr = align(start_addr, 8);
    page_array = (PageBlock *)start_addr;

    // initialize page array as minimum number of blocks
    for(size_t i = 0; i < TOTAL_PAGE; i++){
        PageStatus status = (i % (1 << BLOCK_MAX_ORDER))?
                            PAGE_GROUPED:
                            BLOCK_MAX_ORDER;
        init_block(page_array + i, status, i);
    }
    return 0;
}

//! target region need to be empty. TODO: add security check
void memory_reserve(void *start, size_t size){
    // size_t order = calculate_order(size); // find minumum order.
    addr_t start_page = (addr_t) start & ~(PAGE_SIZE - 1);
    addr_t end_page = ((addr_t) start + size - 1) & ~(PAGE_SIZE - 1);
    
    size_t start_idx = to_block_idx((void *)start_page);
    size_t end_idx = to_block_idx((void *)end_page);
#ifdef PAGE_RESERVE_LOGGER
    send_string("[logger][mem_reserve]: reserving block ");
    send_string(itoa(start_idx, pagelog_buffer, HEX));
    async_send_data('~');
    send_line(itoa(end_idx, pagelog_buffer, HEX));
#endif
    
    while(start_idx <= end_idx){
        size_t sub_block_order = segment_block_seq(start_idx, end_idx - start_idx + 1);

        // split the need chunk form current block
        size_t parent = start_idx;
        while(page_array[parent].status == PAGE_GROUPED) parent &= (parent - 1);
        for(size_t curr_order = page_array[parent].status; curr_order > sub_block_order;){            
            free_list_remove(parent, curr_order);
            size_t buddy = GET_BUDDY(parent, --curr_order);

#ifdef PAGE_RESERVE_LOGGER
            send_string("[logger][mem_reserve]: split ");
            send_string(itoa(parent, pagelog_buffer, HEX));
            send_string(" and ");
            send_string(itoa(buddy, pagelog_buffer, HEX));
            send_string(" with order of ");
            send_line(itoa(curr_order, pagelog_buffer, DEC));
#endif
            //insert the splited block back to free_list
            COND_SWAP(buddy <= start_idx, size_t, parent, buddy);
            page_array[buddy].status = curr_order;
            free_list_insert(page_array + buddy, curr_order);
        }
        
        page_array[start_idx].status = PAGE_RESERVED;
        free_list_remove(start_idx, sub_block_order);
        size_t next_start = start_idx + (1 << sub_block_order);
#ifdef PAGE_RESERVE_LOGGER
        send_string("[logger][mem_reserve]: block ");
        send_string(itoa(start_idx, pagelog_buffer, HEX));
        async_send_data('~');
        send_string(itoa(next_start - 1, pagelog_buffer, HEX));
        send_line(" reserved");
#endif
        start_idx = next_start;   
    }

#ifdef PAGE_RESERVE_LOGGER
    async_send_data('\n');
#endif
}

void *page_alloc(size_t size){
    if(!size) return NULL;

    PageBlock *target = NULL;
    size_t order = calculate_order(size); // find minumum order.

#ifdef PAGE_ALLOC_LOGGER
    send_string("[logger][page_allocator]: request size ");
    send_string(itoa(size, pagelog_buffer, HEX));
#endif

    // find available free space
    for(size_t level = order; level <= BLOCK_MAX_ORDER; level++){
        if(!free_list[level]) continue;
        
        target = free_list_pop(level);
        break;
    }
    if(!target){
        _send_line_("\n[ERROR][page_allocator]: can't find a large enough space", async_send_data);
        return NULL;
    }

#ifdef PAGE_ALLOC_LOGGER
    send_string(", find block ");
    send_string(itoa(target->idx, pagelog_buffer, HEX));
    send_string(" with order ");
    send_line(itoa(target->status, pagelog_buffer, DEC));
#endif

    // split if find a larger block
    while(target->status > order){
        size_t level = --(target->status);
        size_t buddy_idx = GET_BUDDY(target->idx, level);
        page_array[buddy_idx].status = level;
        free_list_insert(page_array + buddy_idx, level);
        
#ifdef PAGE_ALLOC_LOGGER
        send_string("[logger][page_allocator]: split block to ");
        send_string(itoa(target->idx, pagelog_buffer, HEX));
        send_string(" and ");
        send_string(itoa(buddy_idx, pagelog_buffer, HEX));
        send_string(", order ");
        send_line(itoa(level, pagelog_buffer, DEC));
#endif
    }

    target->status = PAGE_OCCUPIED;
    return to_page_address(target);
}

void page_free(void *target){
    addr_t addr = (addr_t) target;
    if(addr % PAGE_SIZE) return; // if not aligned
    
    size_t target_idx = to_block_idx(target);
    PageBlock* target_block = page_array + target_idx;
    if(target_block->status != PAGE_OCCUPIED){ // not freeable
        send_line("[Error][page_allocator]: not an allocated page");
        return; 
    }

#ifdef PAGE_ALLOC_LOGGER
    send_string("[logger][page_allocator]: freeing block ");
    send_line(itoa(target_idx, pagelog_buffer, HEX));
#endif
    
    for(size_t level = 0; level <= BLOCK_MAX_ORDER; level++){
        size_t buddy_idx = GET_BUDDY(target_idx, level);
        // smaller buddy shouldn't be grouped by others,
        if(buddy_idx < target_idx && page_array[buddy_idx].status == PAGE_GROUPED){
            char temp_error_buf[16];
            send_string("[Error][page_allocator]: buddy ");
            send_string(itoa(buddy_idx, temp_error_buf, HEX));
            send_line("shouldn't be grouped");
            return;
        }

        // when buddy isn't empty or is at max level => terminate merging
        if(!BUDDY_MERGEABLE(buddy_idx, level) || level == BLOCK_MAX_ORDER){
#ifdef PAGE_ALLOC_LOGGER
            send_string("[logger][page_allocator]: generate new free block ");
            send_string(itoa(target_idx, pagelog_buffer, HEX));
            send_string(" with order ");
            send_line(itoa(level, pagelog_buffer, DEC));            
            
            if(level != BLOCK_MAX_ORDER){
                send_string("[logger][page_allocator]: terminate because buddy ");
                send_string(itoa(buddy_idx, pagelog_buffer, HEX));
                send_string("'s status is ");
                send_line(itoa(page_array[buddy_idx].status, pagelog_buffer, HEX));
            }
#endif
            page_array[target_idx].status = level;
            free_list_insert(page_array + target_idx, level);
            break;
        }
        
        // else, merge target and buddy
        target_idx = merge_page_block(target_idx, buddy_idx, level);
    }
}

size_t to_block_idx(void *addr){
    addr_t memaddr = (addr_t) addr;
    if(memaddr > MEM_SIZE) {
        _send_line_("\n[ERROR][page_allocator]: address to large, can't be translated", sync_send_data);
        return 0;
    }
    // else if(memaddr % PAGE_SIZE){
    //     _send_line_("[ERROR][page_allocator]: address need to be aligned to page size", sync_send_data);
    //     return 0;
    // }

    return (size_t) memaddr / PAGE_SIZE;
}

// ----- Private Functions -----
void init_block(PageBlock *block, PageStatus status, size_t idx){
    block->status = status;
    block->idx = idx;
    list_init(&block->node);
    free_list_insert(block, status);
}

size_t merge_page_block(size_t target_idx, size_t buddy_idx, size_t level){
#ifdef PAGE_ALLOC_LOGGER
    send_string("[logger][page_allocator]: mergeing ");
    send_string(itoa(target_idx , pagelog_buffer, HEX));
    send_string(" with buddy ");
    send_string(itoa(buddy_idx, pagelog_buffer, HEX));
    send_string(" of status ");
    send_line(itoa(page_array[buddy_idx].status, pagelog_buffer, HEX));
#endif

    if(page_array[buddy_idx].status >= 0) free_list_remove(buddy_idx, level);

    // Smaller idx dominate the merged block
    COND_SWAP(target_idx > buddy_idx, size_t, target_idx, buddy_idx);
    page_array[buddy_idx].status = PAGE_GROUPED;
    return target_idx;
}

// LIFO insert
void free_list_insert(PageBlock *new_block, PageStatus level){
    if(level < 0) return;
    ListNode *next = (free_list[level])? &free_list[level]->node: NULL;
    list_add(&new_block->node, NULL, next);
    free_list[level] = new_block;
}

// remove the first from free_list
PageBlock *free_list_pop(size_t level){
    if(level < 0) return NULL;
    PageBlock *head = free_list[level];
    ListNode *next = list_remove(&head->node);
    free_list[level] = (next)? GET_CONTAINER(next, PageBlock, node): NULL;
    return head;
}

void free_list_remove(size_t idx, size_t level){
    if(level < 0) return;
    if(free_list[level] == page_array + idx){ // reassign head if target is the head of list
        free_list[level] = (page_array[idx].node.next)?
                            GET_CONTAINER(page_array[idx].node.next, PageBlock, node):
                            NULL;
    }

    list_remove(&page_array[idx].node);
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

size_t segment_block_seq(size_t start_idx, size_t size){
    for(size_t order = BLOCK_MAX_ORDER; order >= 0; order--){
        size_t base = 1 << order;
        if(!(start_idx % base) && size >= base){
            return order;
        }
    }
    return -1;
}

void *to_page_address(PageBlock *block){
    return (void *)(block->idx * PAGE_SIZE);
}
