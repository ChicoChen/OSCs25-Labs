#include "simple_shell.h"
#include "simple_commands.h"
#include "mini_uart.h"
#include "str_utils.h"

void simple_shell(){
    char inputline[CMD_BUFFER_SIZE];
    while(1){
        send_line(">:");
        read_cmd_line(inputline);
        exe_cmd(inputline);
    }
}

int read_cmd_line(char *inputline){
    //TODO: length checking
    int writehead = 0;
    while(1){
        char input = read_data();
        if(input == '\r'){ //enter pressed
            inputline[writehead] = '\0';
            send_line("\r\n");
            break;
        }
        else{
            send_data(input);
            inputline[writehead++] = input;
        }
    }
    return writehead;
}

void exe_cmd(char *command){
    int idx = 0;
    while(commands[idx].name != 0){
        if(!strcmp(command, commands[idx].name)){
            idx++;
            continue;
        }
        commands[idx].func(0);
        break;
    }
}