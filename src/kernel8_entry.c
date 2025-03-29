#include "mini_uart.h"
#include "simple_shell.h"
#include "devicetree/dtb.h"
#include "exception/exception.h"
#include "basic_type.h"

#include "str_utils.h"
extern void *_dtb_addr;

void kernel_entry(){
    _init_exception();
    init_uart();
    send_line("----------");
    dtb_parser(find_initramfs, (addr_t)_dtb_addr);
    send_line("hello world");
    simple_shell();
}