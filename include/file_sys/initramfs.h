#ifndef INITRAMFS_H
#define INITRAMFS_H

#include "basic_type.h"

#define LS_BUFFER_SIZE 256
#define MAX_FILENAME 32
#define CAT_BUFFER_SIZE 2048
#define HEADER_SIZE 110

typedef struct{
    char c_magic[6];
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
} cpio_newc_header;

extern char* newc_magic_str;
extern char* terminator;

void init_ramfile();
int list_ramfile(void *args);
int view_ramfile(void *args);

addr_t find_address(char *filename, unsigned int *filesize_ptr);
int exec_usr_prog(void* args);

int check_magic(byte* magic);

#endif