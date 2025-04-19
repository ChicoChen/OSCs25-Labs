#include "allocator/dynamic_allocator.h"
#include "allocator/page_allocator.h"
#include "template/list.h"
#include "mini_uart.h"

#define OBJ_STEP        16
#define OBJ_MIN_SIZE    16
#define OBJ_MAX_SIZE    1024
#define OBJ_ALIGN       16

#define TOTAL_CACHE ((OBJ_MAX_SIZE - OBJ_MIN_SIZE)/ OBJ_STEP + 1)
#define BITMAP_LENGTH (PAGE_SIZE / OBJ_MIN_SIZE) // 256

#define PAGE_HEADER_SIZE (12 + LISTNODE_SIZE + BITMAP_LENGTH) // 284
typedef struct PageHeader{
    uint32_t obj_size;
    uint32_t capacity;
    uint32_t avail;
    ListNode node;
    uint64_t bitmap[BITMAP_LENGTH / 64]; // empty -> 0, occupied -> 1
}PageHeader;

#define ALIGN(target, alignment) ((target + alignment - 1) & ~(alignment - 1))

// ----- local data -----
PageHeader *memory_pool[TOTAL_CACHE];

// ----- forware declarations -----
PageHeader *find_avail_slab(size_t pool_idx);
void *alloc_obj(PageHeader *target_page);

void init_page_header(PageHeader *header, size_t obj_size);
size_t fill_bitmap(PageHeader *target_page);

inline bool is_full(PageHeader *slab);

// ----- public interfaces -----
void init_memory_pool(){
    for(size_t i = 0; i < TOTAL_CACHE; i++){
        memory_pool[i] = NULL;
    }
}

void *kmalloc(size_t size){
    // 
    size_t pool_idx;
    if(size < OBJ_MIN_SIZE) pool_idx = 0;
    else pool_idx = ALIGN(size - OBJ_MIN_SIZE, OBJ_STEP) / OBJ_STEP;

    PageHeader *target_slab = find_avail_slab(pool_idx);
    return alloc_obj(target_slab);
}

void kfree(void *target){

}

// ----- private functions -----

// TODO: currently one slab contains only one page 
//       maybe allocate more than one page for larger obj?
PageHeader *find_avail_slab(size_t pool_idx){
    size_t obj_size = pool_idx * OBJ_STEP + OBJ_MIN_SIZE;
    
    // if no page is allocated
    if(!memory_pool[pool_idx]){
        memory_pool[pool_idx] = (PageHeader *)page_alloc(PAGE_SIZE);
        init_page_header(memory_pool[pool_idx], obj_size);
    }
    
    // finding available slab
    PageHeader *slab = memory_pool[pool_idx];
    while(is_full(slab)) {
        if(slab->node.next == NULL){ // reach the end of slab
            PageHeader *next_slab = (PageHeader *)page_alloc(PAGE_SIZE);
            init_page_header(next_slab, obj_size);
            list_add(&next_slab->node, &slab->node, NULL);
        }
        slab = GET_CONTAINER(slab->node.next, PageHeader, node);
    }
    
    return slab;
}

void *alloc_obj(PageHeader *target_page){
    size_t obj_idx = fill_bitmap(target_page);
    if(obj_idx >= target_page->capacity){
        send_line("[ERROR][dynamic allocator] fill_bitmap() returned idx out-of-range");
        return NULL;        
    }
    target_page->avail--;   

    addr_t page_addr = (addr_t)target_page;
    addr_t element_offset = ALIGN(PAGE_HEADER_SIZE, OBJ_ALIGN);
    return (void *) (page_addr + element_offset + obj_idx * target_page->obj_size);
}

void init_page_header(PageHeader *header, size_t obj_size){
    header->obj_size = obj_size;
    header->capacity = (PAGE_SIZE - ALIGN(PAGE_HEADER_SIZE, OBJ_ALIGN)) / obj_size;
    header->avail = header->capacity;
    
    list_init(&header->node);
    for(size_t i = 0; i < BITMAP_LENGTH / 64; i++){
        header->bitmap[i] = 0;
    }
}

size_t fill_bitmap(PageHeader *target_page){
    size_t total_row = ALIGN(target_page->capacity, 64) / 64;
    for(size_t i = 0; i < total_row; i++){
        uint64_t row_i = target_page->bitmap[i];
        if(row_i == ~0) continue; // all occupied

        for(size_t bit = 0; bit < 64; bit++){
            if(row_i & (0b1) == 0){
                target_page->bitmap[i] |= 1 << bit;
                return i * 64 + bit;
            }
            row_i = row_i >> 1;
        }
    }

    send_line("[ERROR][dynamic allocator] func fill_bitmap() can't find empty space");
    return -1;
}

bool is_full(PageHeader *slab){
    return slab->avail == 0;
}

void *get_first_element(void *page_ptr){
    addr_t page_addr = (addr_t) page_ptr;
    page_addr = page_addr - (page_addr % PAGE_SIZE);
    addr_t element_offset = (PAGE_HEADER_SIZE + OBJ_ALIGN - 1) & ~(OBJ_ALIGN - 1);
    return (void *)(page_addr + element_offset);
}