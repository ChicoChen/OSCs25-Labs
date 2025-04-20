#include "allocator/startup_allocator.h"
#include "allocator/page_allocator.h"
#include "memory_region.h"

void init_mem(){
    init_page_array((void *)PAGE_ARRAY_START);
    memory_reserve((void *)SPIN_TABLE_START, SPIN_TABLE_SIZE);
    memory_reserve((void *)PAGE_ARRAY_START, PAGE_ARRAY_SIZE);
    memory_reserve(&_kernel_start, KERNEL_END - KERNEL_START);
    memory_reserve((void *)USER_PROG_START, USER_STACK_TOP - USER_PROG_START);
    // memory_reserve(_dtb_addr, // dtb_size?);
    // memory_reserve(initramfs_addr, // cpiosize?);
}
