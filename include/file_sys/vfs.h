#ifndef VFS_H
#define VFS_H

#include "basic_type.h"

#define VNODE_SIZE 32
typedef struct {
	Mount* mount;
	VnodeOperations* v_ops;
	FileOperations* f_ops;
	void* internal;
} Vnode;

#define FILEHANDLE_SIZE 24
typedef struct {
	Vnode* vnode;
	size_t f_pos;  // RW position of this file handle
	FileOperations* f_ops;
	int flags;
} FileHandler;

#define MOUNT_SIZE 16
typedef struct {
	Vnode* root;
	FileSystem* fs;
} Mount;

#define FILESYSTEM_SIZE 16
typedef struct {
	const char* name;
	int (*setup_mount)(FileSystem* fs, Mount* mount);
} FileSystem;

#define VNODEOPERATION_SIZE 24
typedef struct {
	int (*lookup)(Vnode* dir_node, Vnode** target,
		const char* component_name);
	int (*create)(Vnode* dir_node, Vnode** target,
		const char* component_name);
	int (*mkdir)(Vnode* dir_node, Vnode** target,
		const char* component_name);
} VnodeOperations;

#define FILE_OPERATIONS_SIZE 40
typedef struct {
	int (*write)(FileHandler* file, const void* buf, size_t len);
	int (*read)(FileHandler* file, void* buf, size_t len);
	int (*open)(Vnode* file_node, FileHandler** target);
	int (*close)(FileHandler* file);
	long (*lseek64)(FileHandler* file, long offset, int whence);
} FileOperations;


int init_vfs();
int init_vnode(Vnode *target, void *internal,
		VnodeOperations *vops, FileOperations *fops);
		
int register_filesystem(FileSystem* fs);

int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, Vnode** target);

int vfs_open(const char* pathname, int flags, FileHandler** target);
int vfs_close(FileHandler* file);
int vfs_write(FileHandler* file, const void* buf, size_t len);
int vfs_read(FileHandler* file, void* buf, size_t len);

#endif