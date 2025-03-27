#include "simple_shell.h"
#include "simple_commands.h"
#include "mini_uart.h"
#include "str_utils.h"

void simple_shell(){
    char inputline[CMD_BUFFER_SIZE];
    char *toks[32];
    while(1){
        send_string(">:");
        echo_read_line(inputline);
        exe_cmd(inputline);
    }
}

void exe_cmd(char *command){
    int idx = 0;
    char *toks[CMD_TOK_MAX_NUM];
    if(!parse_command(command, toks)) return;

    while(commands[idx].name != 0){
        if(!strcmp(toks[0], commands[idx].name)){
            idx++;
            continue;
        }
        commands[idx].func(toks + 1);
        break;
    }
}

char** parse_command(char *commandline, char** toks){
    char *tok = strtok(commandline, " ");
    size_t tok_idx = 0;
    while(tok != NULL && tok_idx < CMD_TOK_MAX_NUM){
        toks[tok_idx++] = tok;
        tok = strtok(NULL, " ");
    }
    toks[tok_idx] = 0; //NULL terminator
    return toks;
}