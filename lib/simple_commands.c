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
#include "allocator/dynamic_allocator.h"

#include "thread/thread.h"

int cmd_help(void* args);
int hello_world(void* args);
int reset(void *arg);
int exec_wrapper(void *args);

int dts_wrapper(void *arg);
int tick_wrapper(void *arg);
int delayed_printline(void *arg);

int demo_page_alloc(void *arg);
int demo_page_free(void *arg);
int demo_dyna_alloc(void *arg);
int demo_dyna_free(void *arg);
int seq_alloc_free(void *arg);

void fork_dummy(void *args);
int kernel_demo(void *arg);

Command commands[] = {
    {"help", cmd_help,          "show the help menu"},
    {"hello", hello_world,      "print \"Hello world!\""},
    {"mailbox", mailbox_entry,  "show mailbox information"},

    {"ls", list_ramfile,        "list files in ramdisk"},
    {"cat", view_ramfile,       "show file content"},
    {"exec", exec_wrapper,     "execute user program in initramfs"},
    {"dts", dts_wrapper,        "show dts content."},
    
    {"tick", tick_wrapper,      "switch on and off timer tick"},
    {"setTimeout", delayed_printline, 
                                "echo inputline after assigned seconds"},
        
    {"memalloc", memalloc,      "allocate memory for a string."},
    {"palloc", demo_page_alloc, "demoing page_alloc())"},
    {"pfree", demo_page_free,   "demoing page_free()"},
    {"kalloc", demo_dyna_alloc, "demoing kmalloc()"},
    {"kfree", demo_dyna_free,   "demoing kfree()"},
    {"memdemo", seq_alloc_free, "demo memory allocation"},
    
    {"threaddemo", kernel_demo, "thread create and scheduling testcase"},
 
    {"reboot", reset,           "reboot the device"},
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

int exec_wrapper(void *args){
    char **arguments = (char **)args;
    char *name = arguments[0];
    char **argv = arguments + 1;
    return exec_user_prog(name, argv);
}

int dts_wrapper(void *arg){
    return dtb_parser(print_dts, (addr_t)_dtb_addr);
}

int tick_wrapper(void *arg){
    char *flag = *(char **)arg;
    bool enable = BOOL(atoi(flag, DEC));
    if(enable) add_timer_event(2, tick_callback, 0);
    else clear_timer_event(tick_callback);
    return 1;
}

int delayed_printline(void *arg){
    char** arguments = (char **)arg;
    size_t message_len = get_size(arguments[0]);
    void *message = simple_alloc(message_len); //! deallocator
    memcpy(message, arguments[0], message_len);
    uint64_t offset = atoi(arguments[1], DEC);
    return add_timer_event(offset, send_void_line, message);
}

int demo_page_alloc(void *arg){
    char *num_page = *((char **)arg);
    size_t allocate_size = (size_t)atoui(num_page, DEC) * PAGE_SIZE;
    void *addr = page_alloc(allocate_size);
    send_string("get address ");
    char temp[16];
    send_line(itoa((unsigned int)addr, temp, HEX));
    return 0;
}

int demo_page_free(void *arg){
    char *page_idx = *((char **)arg);
    size_t target_idx = (size_t)atoui(page_idx, HEX);
    page_free((void *)(target_idx * PAGE_SIZE));
    return 0;
}

int demo_dyna_alloc(void *arg){
    char *queried_size = *((char **)arg);
    size_t query = (size_t)atoui(queried_size, DEC);
    void *addr = kmalloc(query);
    send_string("\nget address ");
    char temp[16];
    send_line(itoa((unsigned int)addr, temp, HEX));
    return 0;
}

int demo_dyna_free(void *arg){
    char *target_address = *((char **)arg);
    void *query = (void *)atoui(target_address, HEX);
    kfree(query);
    return 0;
}

int seq_alloc_free(void *arg){
    void **addrs;
    addrs = page_alloc(256 * 8);
    for(size_t i = 1; i < 256; i++){
        addrs[i] = page_alloc(i * PAGE_SIZE);
    }

    for(size_t i = 1; i < 256; i++){
        page_free(addrs[i]);
    }

    return 0;
}

void fork_dummy(void *args){
    char buffer[16];
    for(int i = 0; i < 10; ++i) {
        send_string("Thread id: ");
        send_string(itoa(get_current_id(), buffer, HEX));
        send_string(" ");
        send_line(itoa(i, buffer, HEX));
        delay(1000000);
        schedule();
    }
    NULL;
}

int kernel_demo(void *args) {
    for(int i = 0; i < 4; ++i) {
        make_thread(fork_dummy, NULL);
    }
    idle();

    return 0;
}