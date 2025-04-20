#include "mini_uart.h"
#include "simple_shell.h"
#include "file_sys/initramfs.h"
#include "exception/exception.h"
#include "timer/timer.h"
#include "allocator/page_allocator.h"
#include "basic_type.h"

#include "str_utils.h"

void kernel_entry(){
    init_exception();
    init_page_array((void *)PAGE_ARRAY_START);
    init_core_timer();
    init_uart();
    send_line("--------------------");
    init_ramfile();
    send_line("hello world");
    simple_shell();
}