#include "simple_shell.h"
#include "simple_commands.h"
#include "mini_uart.h"
#include "str_utils.h"

void simple_shell(){
    char inputline[CMD_BUFFER_SIZE];
    while(1){
        send_string(">:");
        echo_read_line(inputline);
        exe_cmd(inputline);
    }
}

void exe_cmd(char *command){
    int idx = 0;
    while(commands[idx].name != 0){
        if(!strcmp(command, commands[idx].name)){
            idx++;
            continue;
        }

        //TODO: command parser
        commands[idx].func(0);
        break;
    }
}