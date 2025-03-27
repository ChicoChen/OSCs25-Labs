#ifndef SIMPLE_SHELL_H
#define SIMPLE_SHELL_H

#define CMD_BUFFER_SIZE 256
#define CMD_TOK_MAX_NUM 32

void simple_shell();
int read_cmd_line(char *command);
void exe_cmd(char *command);
char** parse_command(char *commandline, char** toks);
#endif