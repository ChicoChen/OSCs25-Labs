#ifndef INITRAMFS_H
#define INITRAMFS_H

#include "basic_type.h"
#include "allocator/rc_region.h"
#include "file_sys/vfs.h"

#define LS_BUFFER_SIZE 256
#define MAX_FILENAME 32
#define CAT_BUFFER_SIZE 2048
#define HEADER_SIZE 110

#define INITRAMFS_MAX_FILESIZE 2048
#define INITRAMFS_MAX_CHILDREN_NUM 32

extern FileSystem initramfs;
extern size_t initramfs_size;

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

typedef enum {
    content_file,
    directory
} InitramfsType;

typedef struct {
    char *child_name;
    Vnode *children;
}InitramfsChildNode;

typedef union {
    char file_content[INITRAMFS_MAX_FILESIZE];
    InitramfsChildNode *children[INITRAMFS_MAX_CHILDREN_NUM];
} InitramfsData;

typedef union {
    size_t num_children;
    size_t filesize;
} InitramfsDataSize;

typedef struct{
    InitramfsType type;
    Vnode *parent;
    InitramfsDataSize data_size;
    InitramfsData *data;
}InitramfsInternal;

extern char* newc_magic_str;
extern char* terminator;

void get_initramfs_info();
int list_ramfile(void *args);
int view_ramfile(void *args);

void init_initramfs();
int mount_initramfs(FileSystem *fs, Mount *mount);

RCregion *load_program(char *prog_name);
int run_prog(char *name, char **argv);

#endif