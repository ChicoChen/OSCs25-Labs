#include "allocator/dynamic_allocator.h"
#include "allocator/page_allocator.h"
#include "template/list.h"
#include "mini_uart.h"
#include "str_utils.h"
#include "utils.h"

#define ALIGN(target, alignment) ((target + alignment - 1) & ~(alignment - 1))

#define OBJ_STEP        16
#define OBJ_MIN_SIZE    16
#define OBJ_MAX_SIZE    1024
#define OBJ_ALIGN       16

#define TOTAL_CACHE ((OBJ_MAX_SIZE - OBJ_MIN_SIZE)/ OBJ_STEP + 1)
#define BITMAP_LENGTH (PAGE_SIZE / OBJ_MIN_SIZE) // 256

#define PAGE_HEADER_SIZE    (12 + LISTNODE_SIZE + BITMAP_LENGTH) // 284
#define ELEMENT_OFFSET      ALIGN(PAGE_HEADER_SIZE, OBJ_ALIGN)  // 288
typedef struct PageHeader{
    uint32_t obj_size;
    uint32_t capacity;
    uint32_t avail;
    ListNode node;
    uint64_t bitmap[BITMAP_LENGTH / 64]; // empty -> 0, occupied -> 1
}PageHeader;

// ----- local data -----
PageHeader *memory_pool[TOTAL_CACHE];
#ifdef DYNAMIC_ALLOC_LOGGER
char logger_buffer[32];
#endif

// ----- forware declarations -----
void init_page_header(PageHeader *header, size_t obj_size);
void clear_page_header(PageHeader *header);


PageHeader *find_avail_slab(size_t pool_idx);
void *alloc_obj(PageHeader *target_page);
bool free_obj(void* target_addr, PageHeader* slab);

size_t fill_bitmap(PageHeader *target_page);
inline size_t get_pool_idx(size_t obj_size);

inline bool is_full(PageHeader *slab);

// ----- public interfaces -----
void init_memory_pool(){
    for(size_t i = 0; i < TOTAL_CACHE; i++){
        memory_pool[i] = NULL;
    }
}

void *dyna_alloc(size_t size){
    size_t pool_idx = get_pool_idx(size);
#ifdef DYNAMIC_ALLOC_LOGGER
    send_string("[logger][dynamic_alloc]: dyna_alloc() size ");
    send_string(itoa(size, logger_buffer, DEC));
#endif

    PageHeader *target_slab = find_avail_slab(pool_idx);
    if(!target_slab) return NULL;
    
    return alloc_obj(target_slab);
}

void dyna_free(void *target){
    if(!is_allocated(target)){
        send_line("[ERROR][dynamic alloc]: address is not allocated by dyna_allocator");
        return;
    }
    size_t page_idx = to_block_idx(target);
    PageHeader *slab = (PageHeader *)to_page_address(page_idx);
    if(!free_obj(target, slab)) return;

    if(slab->avail == slab->capacity
        && memory_pool[get_pool_idx(slab->obj_size)] != slab)
    {
#ifdef DYNAMIC_ALLOC_LOGGER
        send_line("[logger][dynamic_alloc]: freeing empty slab");
#endif
        // clear_page_header(slab); //don't really need to since no other module use page_alloc
        page_free((void *)slab);
    }
}

// ----- private functions -----

// TODO: currently one slab contains only one page 
//       maybe allocate more than one page for larger obj?

void init_page_header(PageHeader *header, size_t obj_size){
    header->obj_size = obj_size;
    header->capacity = (PAGE_SIZE - ALIGN(PAGE_HEADER_SIZE, OBJ_ALIGN)) / obj_size;
    header->avail = header->capacity;
    
    node_init(&header->node);
    for(size_t i = 0; i < BITMAP_LENGTH / 64; i++){
        header->bitmap[i] = 0;
    }
}

void clear_page_header(PageHeader *header){
    header->obj_size = 0;
    header->capacity = 0;
    header->avail = 0;
    
    node_init(&header->node);
    for(size_t i = 0; i < BITMAP_LENGTH / 64; i++){
        header->bitmap[i] = 0;
    }
}

PageHeader *find_avail_slab(size_t pool_idx){
    if(pool_idx >= TOTAL_CACHE) return NULL;
    size_t obj_size = pool_idx * OBJ_STEP + OBJ_MIN_SIZE;
#ifdef DYNAMIC_ALLOC_LOGGER
    send_string(", pool-");
    send_line(itoa(obj_size, logger_buffer, DEC));        
#endif

// if no page is allocated
    if(!memory_pool[pool_idx]){
#ifdef DYNAMIC_ALLOC_LOGGER
        send_line("[logger][dynamic_alloc]: allocating new slab");
#endif
        memory_pool[pool_idx] = (PageHeader *)page_alloc(PAGE_SIZE);
        init_page_header(memory_pool[pool_idx], obj_size);
    }
    
    // finding available slab
    PageHeader *slab = memory_pool[pool_idx];
    while(is_full(slab)) {
        if(slab->node.next == NULL){ // reach the end of slab
#ifdef DYNAMIC_ALLOC_LOGGER
            send_string("[logger][dynamic_alloc]: alloc new slab for pool-");
            send_line(itoa(obj_size, logger_buffer, DEC));
#endif
            PageHeader *next_slab = (PageHeader *)page_alloc(PAGE_SIZE);
            init_page_header(next_slab, obj_size);
            list_add(&next_slab->node, &slab->node, NULL);
        }
        slab = GET_CONTAINER(slab->node.next, PageHeader, node);
    }

#ifdef DYNAMIC_ALLOC_LOGGER
    send_string("[logger][dynamic_alloc]: find avail space in slab ");
    send_line(itoa(to_block_idx((void *) slab), logger_buffer, HEX));
#endif
    return slab;
}

void *alloc_obj(PageHeader *target_page){
    size_t obj_idx = fill_bitmap(target_page);
    if(obj_idx >= target_page->capacity){
        send_line("[ERROR][dynamic_alloc] fill_bitmap() returned idx out-of-range");
        return NULL;        
    }
    target_page->avail--;   

    addr_t page_addr = (addr_t)target_page;
    void *obj_address = (void *) (page_addr + ELEMENT_OFFSET + obj_idx * target_page->obj_size);
#ifdef DYNAMIC_ALLOC_LOGGER
    send_string("[logger][dynamic_alloc]: allocate object at idx ");
    send_string(itoa(obj_idx, logger_buffer, DEC));
    send_string("(address: ");
    send_string(itoa((uint32_t)obj_address, logger_buffer, HEX));
    send_string("), remained slot: ");
    send_line(itoa(target_page->avail, logger_buffer, DEC));
#endif

    return obj_address;
}

bool free_obj(void* target_addr, PageHeader* slab){
    size_t obj_size = slab->obj_size;
    size_t target_offset = (addr_t)target_addr - (addr_t)slab;
    if(target_offset <  ELEMENT_OFFSET) {
        send_line("[ERROR][dynamic alloc]: addr invalid, overlapped with slab header");
        return false;
    }
    
    size_t obj_idx = (target_offset - ELEMENT_OFFSET) / obj_size;
    if(obj_idx > slab->capacity){
        send_line("[ERROR][dynamic alloc]: addr invalid, exceed slab's capacity");
        return false;
    }

    slab->bitmap[obj_idx/64] &= ~(1 << (obj_idx % 64));
    slab->avail++;
#ifdef DYNAMIC_ALLOC_LOGGER
    send_string("[logger][dynamic_alloc]: erased object at slot ");
    send_string(itoa(obj_idx, logger_buffer, DEC));
    send_string(" remaining objects: ");
    send_line(itoa(slab->capacity - slab->avail, logger_buffer, DEC));
#endif
    return true;
}

size_t fill_bitmap(PageHeader *target_page){
    size_t total_row = ALIGN(target_page->capacity, 64) / 64;
    for(size_t i = 0; i < total_row; i++){
        uint64_t row_i = target_page->bitmap[i];
        if(row_i == ~0) continue; // all occupied

        for(size_t bit = 0; bit < 64; bit++){
            if((row_i & (0b1)) == 0){
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

size_t get_pool_idx(size_t obj_size){
    return (obj_size < OBJ_MIN_SIZE)
            ? 0
            : ALIGN(obj_size - OBJ_MIN_SIZE, OBJ_STEP) / OBJ_STEP;
}