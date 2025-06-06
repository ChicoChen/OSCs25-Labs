#ifndef TMPFS_H
#define TMPFS_H

#include "file_sys/vfs.h"
#include "basic_type.h"

#define FILE_NAME_LENGTH        (15 + 1)
#define FILE_CONTENT_LENGTH     4096 // a page
#define MAX_FILE_ENTRY_SIZE     16

typedef enum {
    content_file,
    directory
} TmpfsType;


#define TMPFSINTERNAL_SIZE (32 + 8 * MAX_FILE_ENTRY_SIZE)
typedef struct {
    TmpfsType type;
    Vnode *parent;
    size_t num_children;
    char **children_name;
    Vnode *children[MAX_FILE_ENTRY_SIZE];
    size_t file_size;
    void *content; // only used if type == content_file
} TmpfsInternal;

FileSystem tmpfs;

void init_tmpfs();
int mount_tmpfs(FileSystem* fs, Mount* mount);

int tmpfs_lookup_i(Vnode* dir_node, Vnode** target, const char* component_name);
int tmpfs_create_i(Vnode* dir_node, Vnode** target, const char* component_name);
int tmpfs_mkdir_i(Vnode* dir_node, Vnode** target, const char* component_name);

int tmpfs_open_i(Vnode* file_node, FileHandler** target);
int tmpfs_read_i(FileHandler* file, void* buf, size_t len);
int tmpfs_write_i(FileHandler* file, const void* buf, size_t len);
int tmpfs_close_i(FileHandler* file);

#endif