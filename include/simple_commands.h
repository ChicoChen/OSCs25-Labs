#ifndef SIMPLE_COMMANDS_H
#define SIMPLE_COMMANDS_H

#define CMD_BUFFER_SIZE 64


typedef struct{
    char *name;
    int (*func)(void*);
    char* description;
} Command;

int cmd_help(void* args);
int hello_world(void* args);

extern Command commands[];
#endif