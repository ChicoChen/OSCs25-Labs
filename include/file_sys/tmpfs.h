#ifndef TMPFS_H
#define TMPFS_H

#include "file_sys/vfs.h"
#include "basic_type.h"

#define FILE_NAME_LENGTH        (15 + 1)
#define FILE_CONTENT_LENGTH     4096 // a page
#define MAX_FILE_ENTRY_SIZE     16

#define TMPFSINTERNAL_SIZE (24 + 8 * MAX_FILE_ENTRY_SIZE)
typedef struct {
    Filetype type;
    size_t num_children;
    char **children_name;
    Vnode *children[MAX_FILE_ENTRY_SIZE];
    void *content; // only used if type == content_file
} TmpfsInternal;

FileSystem tmpfs;

void init_tmpfs();
int mount_tmpfs(FileSystem* fs, Mount* mount);

int tmpfs_mkdir(const char* pathname);
int tmpfs_lookup(const char* pathname, Vnode** target);

int tmpfs_open(const char* pathname, int flags, FileHandler** target);
int tmpfs_close(FileHandler* file);
int tmpfs_write(FileHandler* file, const void* buf, size_t len);
int tmpfs_read(FileHandler* file, void* buf, size_t len);

#endif