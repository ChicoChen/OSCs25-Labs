#include "mini_uart.h"
#include "simple_shell.h"

void sys_entry(){
    init_uart();
    send_line("hello world\r\n");
    simple_shell();
}