#include "mini_uart.h"
#include "simple_shell.h"

void kernel_entry(){
    init_uart();
    send_line("hello world");
    simple_shell();
}