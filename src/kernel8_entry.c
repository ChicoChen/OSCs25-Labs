#include "mini_uart.h"
#include "simple_shell.h"
#include "exception/exception.h"
#include "timer/timer.h"
#include "allocator/startup_allocator.h"
#include "thread/thread.h"
#include "basic_type.h"

#include "str_utils.h"

void kernel_entry(){
    init_exception();
    init_core_timer();
    init_uart();
    init_mem();
    init_thread_sys();
    send_line("--------------------");
    send_line("hello world");
    simple_shell();
}