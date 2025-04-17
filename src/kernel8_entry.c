#include "mini_uart.h"
#include "simple_shell.h"
#include "devicetree/dtb.h"
#include "exception/exception.h"
#include "timer/timer.h"
#include "allocator/page_allocator.h"
#include "basic_type.h"

#include "str_utils.h"
extern void *_dtb_addr;

void kernel_entry(){
    init_exception();
    init_page_array((void *)PAGE_ARRAY_START);
    init_core_timer();
    init_uart();
    send_line("--------------------");
    dtb_parser(find_initramfs, (addr_t)_dtb_addr);
    send_line("hello world");
    simple_shell();
}