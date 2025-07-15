#ifndef INITRAMFS_H
#define INITRAMFS_H

#include "basic_type.h"
#include "allocator/rc_region.h"
#include "file_sys/vfs.h"

#define LS_BUFFER_SIZE 256
#define CAT_BUFFER_SIZE 2048

#define INITRAMFS_MAX_PATH_LEN 128
// wait, the meta of CPIO files is known, I don't need max limitation.
// // #define INITRAMFS_MAX_FILENAME_LEN 64
// // #define INITRAMFS_MAX_FILESIZE 
// // #define INITRAMFS_MAX_CHILDREN_NUM 16

extern FileSystem initramfs;
extern size_t initramfs_size;

#define CPIO_HEADER_SIZE 110
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
} CpioNewcHeader;

typedef enum {
    content_file,
    directory
} InitramfsType;

typedef struct {
    char *name;
    Vnode *node;
}InitramfsChild;

typedef union {
    char *file_content;
    InitramfsChild **children;
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