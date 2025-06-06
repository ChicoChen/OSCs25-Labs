#include "file_sys/vfs.h"
#include "file_sys/tmpfs.h"
#include "file_sys/fs_macros.h"
#include "allocator/dynamic_allocator.h"

#define FILESYSTEM_MAX_NUM 10

// ----- local & forward declarations -----

Mount rootfs;
Vnode *vnode_iter;
FileSystem *registered_fs[FILESYSTEM_MAX_NUM];
size_t num_registerd_fs = 0;

// ----- public interfaces -----
int init_vfs(){
	init_tmpfs();
	register_filesystem(&tmpfs);
	tmpfs.setup_mount(&tmpfs, &rootfs);
}

int init_vnode(Vnode *target, Mount *mount, void *internal,
	VnodeOperations *vops, FileOperations *fops)
{
	target->mount = mount;
	target->internal = internal;
	target->vops = vops;
	target->vops = fops;
}

int register_filesystem(FileSystem* fs) {
	// register the file system to the kernel.
	// you can also initialize memory pool of the file system here.
	if(num_registerd_fs >= FILESYSTEM_MAX_NUM) return -1;
	registered_fs[num_registerd_fs++] = fs;
}

// ----- public vnode operations -----
int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);


int vfs_lookup(const char* pathname,  Vnode** target){

}

// ----- public file operations -----
int vfs_open(const char* pathname, int flags, FileHandler** target) {
	// Lookup pathname
	Vnode *vnode = NULL;
	int error = vfs_lookup(pathname, &vnode);
	if(error == FILE_NOT_FOUND) {
		if(flags & O_CREAT == 0) return error;
		
		// create a new file since O_CREAT is specified
		// vnode = ...
	}
	else if(error < 0) return error; // other errors

	// Create a new file handle for this vnode if found.
	// if(!vnode->fops) ...
	error = vnode->fops->open(vnode, target);
	if(error < 0) return error;

	(*target)->flags = flags;
	return 0;
}

int vfs_close(FileHandler* file) {
	// 1. release the file handle
	// 2. Return error code if fails
}

int vfs_write(FileHandler* file, const void* buf, size_t len) {
	// 1. write len byte from buf to the opened file.
	// 2. return written size or error code if an error occurs.
}

int vfs_read(FileHandler* file, void* buf, size_t len) {
	// 1. read min(len, readable size) byte to buf from the opened file.
	// 2. block if nothing to read for FIFO type
	// 2. return read size or error code if an error occurs.
}
