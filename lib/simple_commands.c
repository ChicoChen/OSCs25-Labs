#include "simple_commands.h"
#include "mini_uart.h"
#include "utils.h"

#include "mailbox/mailbox.h"
#include "file_sys/initramfs.h"
#include "allocator/simple_alloc.h"
#include "devicetree/dtb.h"

extern void *_dtb_addr;

Command commands[] = {
    {"help", cmd_help, "show the help menu"},
    {"hello", hello_world, "print \"Hello world!\""},
    {"mailbox", mailbox_entry, "show mailbox information"},
    {"ls", list_ramfile, "list files in ramdisk"},
    {"cat", view_ramfile, "show file content"},
    {"memalloc", memalloc, "allocate memory for a string."},
    {"dts", dts_wrapper, "show dts content."},
    {"exec", exec_usr_prog, "execute user program in initramfs (target currently fixed)"},
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

int dts_wrapper(void *arg){
    return dtb_parser(find_initramfs, (addr_t)_dtb_addr);
}

int hello_world(void *arg){
    send_line("Hello world!");
    return 0;
}

int reset(void *arg) {        
    // int tick = *(int *)arg;
    send_line("rebooting...");
    int tick = 10;
    set(PM_RSTC, PM_PASSWORD | 0x20);
    set(PM_WDOG, PM_PASSWORD | tick);
}