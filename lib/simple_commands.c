#include "simple_commands.h"
#include "memory_region.h"
#include "mini_uart.h"
#include "utils.h"
#include "str_utils.h"

#include "mailbox/mailbox.h"
#include "file_sys/initramfs.h"
#include "allocator/simple_alloc.h"
#include "devicetree/dtb.h"
#include "timer/timer.h"

#include "allocator/page_allocator.h"

Command commands[] = {
    {"help", cmd_help, "show the help menu"},
    {"hello", hello_world, "print \"Hello world!\""},
    {"mailbox", mailbox_entry, "show mailbox information"},
    {"ls", list_ramfile, "list files in ramdisk"},
    {"cat", view_ramfile, "show file content"},
    {"memalloc", memalloc, "allocate memory for a string."},
    {"dts", dts_wrapper, "show dts content."},
    {"exec", exec_usr_prog, "execute user program in initramfs (target currently fixed)"},
    {"tick", tick_wrapper, "switch on and off timer tick"},
    {"setTimeout", delayed_printline, "echo inputline after assigned seconds"},
    {"demo", demo_page, "demoing page allocation"},
    {"free", demo_free, "demoing page free"},
    {"reboot", reset, "reboot the device"},
    {0, 0, 0} //terminator
};

int cmd_help(void *arg){
    int idx = 0;
    while(commands[idx].name != 0){
        send_string(commands[idx].name);
        send_string(":  ");
        send_line(commands[idx].description);
        idx++;
    }
    return 0;
}

int hello_world(void *arg){
    send_line("Hello world!");
    return 0;
}

int reset(void *arg) {        
    // int tick = *(int *)arg;
    send_line("rebooting...");
    int tick = 10;
    addr_set(PM_RSTC, PM_PASSWORD | 0x20);
    addr_set(PM_WDOG, PM_PASSWORD | tick);
    return 0;
};

int dts_wrapper(void *arg){
    return dtb_parser(print_dts, (addr_t)_dtb_addr);
}

int tick_wrapper(void *arg){
    char *flag = *(char **)arg;
    bool enable = BOOL(atoi(flag, DEC));
    if(enable) add_event(2, tick_callback, 0);
    else timer_clear_event(tick_callback);
    return 1;
}

int delayed_printline(void *arg){
    char** arguments = (char **)arg;
    size_t message_len = get_size(arguments[0]);
    void *message = simple_alloc(message_len); //! deallocator
    memcpy(message, arguments[0], message_len);
    uint64_t offset = atoi(arguments[1], DEC);
    return add_event(offset, send_void_line, message);
}


int demo_page(void *arg){
    char *num_page = *((char **)arg);
    size_t allocate_size = (size_t)atoui(num_page, DEC) * PAGE_SIZE;
    void *addr = page_alloc(allocate_size);
    send_string("get address ");
    char temp[16];
    send_line(itoa((unsigned int)addr, temp, HEX));
    return 0;
}

int demo_free(void *arg){
    char *page_idx = *((char **)arg);
    size_t target_idx = (size_t)atoui(page_idx, HEX);
    page_free((void *)(target_idx * PAGE_SIZE));
    return 0;
}