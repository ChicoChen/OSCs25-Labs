#include "simple_commands.h"
#include "mini_uart.h"
#include "mailbox/mailbox.h"
#include "utils.h"

Command commands[] = {
    {"help", cmd_help, "print this help menu"},
    {"hello", hello_world, "print Hello world!"},
    {"reboot", reset, "reboot the device"},
    {"mailbox", mailbox_entry, "show mailbox information"},
    {0, 0, 0} //terminator
};

int cmd_help(void *arg){
    int idx = 0;
    while(commands[idx].name != 0){
        send_line(commands[idx].name);
        send_line("\t: ");
        send_line(commands[idx].description);
        send_line("\r\n");
        idx++;
    }
    return 0;
}

int hello_world(void *arg){
    send_line("Hello world!\r\n");
    return 0;
}

int reset(void *arg) {        
    // int tick = *(int *)arg;
    send_line("rebooting...\n");
    int tick = 10;
    set(PM_RSTC, PM_PASSWORD | 0x20);
    set(PM_WDOG, PM_PASSWORD | tick);
}