#include "file_sys/vfs.h"
#include "file_sys/tmpfs.h"
#include "file_sys/fs_macros.h"
#include "allocator/dynamic_allocator.h"
#include "thread/thread.h"
#include "utils.h"
#include "str_utils.h"

#define FILESYSTEM_MAX_NUM 10

// ----- local & forward declarations -----
Mount rootfs;
Vnode *vnode_iter;
FileSystem *registered_fs[FILESYSTEM_MAX_NUM];
size_t num_registerd_fs = 0;

FileSystem* fs_registered(const char* fs_name);

// ----- public interfaces -----
int init_vfs(){
	init_tmpfs();
	register_filesystem(&tmpfs);
	rootfs.parent = NULL;

	tmpfs.setup_mount(&tmpfs, &rootfs);
	return 0;
}

int init_vnode(Vnode *target, Mount *mount, void *internal,
	VnodeOperations *vops, FileOperations *fops)
{
	target->mount = mount;
	target->internal = internal;
	target->vops = vops;
	target->fops = fops;
	return 0;
}

int register_filesystem(FileSystem* fs) {
	if(num_registerd_fs >= FILESYSTEM_MAX_NUM) return OPERATION_NOT_ALLOW;
	registered_fs[num_registerd_fs++] = fs;
	return 0;
}

// ----- public vnode operations -----
int vfs_mkdir(const char* pathname){
	char *path = (char *)dyna_alloc(PATHANME_MAX_LENGTH);
	memcpy((void *)path, (void *)pathname, get_size(pathname));
	
	Vnode *parent;
	if(path[0] == '/'){
		parent = rootfs.root;
	}
	else parent = get_curr_thread()->cwd;

	char *saveptr = NULL;
	char *token = strtok_r(path, "/", &saveptr);
	if(!token){
		dyna_free((void *)path);
		return OPERATION_NOT_ALLOW; // trying to create current dir?
	}
	
	Vnode *target;
	do{	
		if(parent->mount && strcmp(token, "..")) {
			parent = parent->mount->parent;
		}

		int lookup = parent->vops->lookup(parent, &target, token);
		if(lookup == FILE_NOT_FOUND){
			parent->vops->mkdir(parent, &target, token);
		}
		else if(lookup != 0) {
			dyna_free((void *)path);
			return lookup;
		}

		parent = (target->mount)? target->mount->root: target;
	}while(token = strtok_r(NULL, "/", &saveptr));
	
	dyna_free((void *)path);
	return 0;
}

int vfs_mount(const char* target, const char* filesystem){
	FileSystem *registered_fs = fs_registered(filesystem);
	if(!registered_fs) return OPERATION_NOT_ALLOW;

	Vnode *mounted_point = NULL;
	int lookup = vfs_lookup(target, &mounted_point);
	if(lookup < 0) return lookup;

	if(mounted_point->mount != NULL){ // already mounted
		return OPERATION_NOT_ALLOW;
		// or unmount original fs and mount a new one
		// but I have yet to implement a destructor
	}

	mounted_point->mount = dyna_alloc(MOUNT_SIZE);
	if(!mounted_point->mount) return ALLOCATION_FAILED;
	mounted_point->mount->parent = mounted_point;
	return registered_fs->setup_mount(registered_fs, mounted_point->mount);
}

int vfs_lookup(const char* pathname,  Vnode** target){
	char *path = (char *)dyna_alloc(PATHANME_MAX_LENGTH);
	memcpy((void *)path, (void *)pathname, get_size(pathname));
	
	if(path[0] == '/') *target = rootfs.root;
	else *target = get_curr_thread()->cwd;

	char *saveptr = NULL;
	char *token = strtok_r(path, "/", &saveptr);
	if(!token) {
		dyna_free((void *)path);
		return 0;
	}
	
	Vnode *next;
	do{
		if((*target)->mount && strcmp(token, "..")){
			*target = (*target)->mount->parent;
		}

		int lookup = (*target)->vops->lookup(*target, &next, token);
		if(lookup != 0){
			dyna_free((void *)path);
			return lookup;
		}
		else *target = (next->mount)? next->mount->root: next;
	}
	while(token = strtok_r(NULL, "/", &saveptr));
	
	dyna_free((void *)path);
	return 0;
}

// ----- public file operations -----
int vfs_open(const char* pathname, int flags, FileHandler** target) {
	// copy pathname
	Vnode *parent;
	char *path = (char *)dyna_alloc(PATHANME_MAX_LENGTH);
	memcpy((void *)path, (void *)pathname, get_size(pathname));
	
	// find starting point
	if(path[0] == '/') parent = rootfs.root;
	else parent = get_curr_thread()->cwd;

	char *savepath;
	char *curr_tok = strtok_r(path, "/", &savepath);
	if(!curr_tok) return parent->fops->open(parent, target);

	// iterate file tree
	char *next_tok;
	Vnode *child = NULL;
	int error;
	while(next_tok = strtok_r(NULL, "/", &savepath)){
		if(strcmp(curr_tok, "..") && parent->mount){
			parent = parent->mount->parent;
		}

		error = parent->vops->lookup(parent, &child, curr_tok);
		if(error < 0){
			dyna_free(path);
			return error;
		}

		parent = (child->mount)? child->mount->root: child;
		curr_tok = next_tok;
	}

	// handle last token
	if(strcmp(curr_tok, "..") && parent->mount){
		parent = parent->mount->parent;
	}
	error = parent->vops->lookup(parent, &child, curr_tok);
	if(error == FILE_NOT_FOUND && (flags & O_CREAT)){ // create new file;
		error = parent->vops->create(parent, &child, curr_tok);
	}

	dyna_free(path);
	if(error < 0) return error;
	child = (child->mount)? child->mount->root: child;
	error = child->fops->open(child, target);
	(*target)->flags = flags;
	return error;
}

int vfs_read(FileHandler* file, void* buf, size_t len) {
	// ! block if nothing to read for FIFO type
	return file->vnode->fops->read(file, buf, len);
}

int vfs_write(FileHandler* file, const void* buf, size_t len) {
	return file->vnode->fops->write(file, buf, len);
}

int vfs_close(FileHandler* file) {
	return file->vnode->fops->close(file);
}

// ----- local methods -----
FileSystem* fs_registered(const char* fs_name){
	for(size_t i = 0; i < num_registerd_fs; i++){
		if(!strcmp(registered_fs[i]->name, fs_name)) continue;
		return registered_fs[i];
	}
	return NULL;
}
