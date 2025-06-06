#ifndef VFS_H
#define VFS_H

#include "basic_type.h"

typedef struct Vnode 				Vnode;
typedef struct Mount 				Mount;
typedef struct FileSystem 			FileSystem;
typedef struct FileHandler 			FileHandler;
typedef struct VnodeOperations 		VnodeOperations;
typedef struct FileOperations 		FileOperations;

#define VNODE_SIZE 32
struct Vnode{
	Mount* mount;
	VnodeOperations* vops;
	FileOperations* fops;
	void* internal;
};

#define FILEHANDLER_SIZE 24
struct FileHandler{
	Vnode* vnode;
	size_t f_pos;  // RW position of this file handle
	FileOperations* ops;
	int flags;
};

#define MOUNT_SIZE 16
struct Mount {
	Vnode* root;
	FileSystem* fs;
};

#define FILESYSTEM_SIZE 16
struct FileSystem{
	const char* name;
	int (*setup_mount)(FileSystem* fs, Mount* mount);
};

#define VNODEOPERATION_SIZE 24
struct VnodeOperations{
	int (*lookup)(Vnode* dir_node, Vnode** target,
		const char* component_name);
	int (*create)(Vnode* dir_node, Vnode** target,
		const char* component_name);
	int (*mkdir)(Vnode* dir_node, Vnode** target,
		const char* component_name);
};

#define FILE_OPERATIONS_SIZE 40
struct FileOperations {
	int (*write)(FileHandler* file, const void* buf, size_t len);
	int (*read)(FileHandler* file, void* buf, size_t len);
	int (*open)(Vnode* file_node, FileHandler** target);
	int (*close)(FileHandler* file);
	long (*lseek64)(FileHandler* file, long offset, int whence);
};


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