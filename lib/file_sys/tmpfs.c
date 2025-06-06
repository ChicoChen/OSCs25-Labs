#include "file_sys/tmpfs.h"
#include "file_sys/fs_macros.h"
#include "allocator/dynamic_allocator.h"
#include "utils.h"

VnodeOperations tmpfs_vops;
FileOperations tmpfs_fops;

// ----- forward declarations -----
void init_tmpfs_ops();


// ----- public functions -----
void init_tmpfs(){
    tmpfs.name = (char *)dyna_alloc(6);
    memcpy(tmpfs.name, "tmpfs", 6);
    tmpfs.setup_mount = mount_tmpfs;
    init_ops();
}

/// @brief setup_mount() of tmpfs
int mount_tmpfs(FileSystem* fs, Mount* mount){
    if(!fs || !mount) return -1;
    mount->fs = fs;
    mount->root = (Vnode *)dyna_alloc(VNODE_SIZE);
    
    // create internal node for root
    TmpfsInternal *tmpfs_node = (TmpfsInternal*) dyna_alloc(TMPFSINTERNAL_SIZE);
    init_tmpfs_node(tmpfs_node);
    init_vnode(mount->root, (void *)tmpfs_node, &tmpfs_vops, &tmpfs_fops);
}

int tmpfs_write(FileHandler* file, const void* buf, size_t len){

}

int tmpfs_read(FileHandler* file, void* buf, size_t len){

}

int tmpfs_open(Vnode* file_node, FileHandler** target){
    FileHandler *new_fd = (FileHandler *)dyna_alloc(FILEHANDLER_SIZE);
    if(!new_fd) return ALLOCATION_FAILED;

    new_fd->vnode = file_node;
    new_fd->f_pos = 0;
    new_fd->ops = &tmpfs_fops;
    *target = new_fd;
    return 0;
}

int tmpfs_close(FileHandler* file){

}

long tmpfs_lseek64(FileHandler* file, long offset, int whence){

}


void init_tmpfs_ops(){
    // vops
    

    // fops
    tmpfs_fops.open = (void *)tmpfs_open;
    tmpfs_fops.read = (void *)tmpfs_read;
    tmpfs_fops.write = (void *)tmpfs_write;
    tmpfs_fops.close = (void *)tmpfs_close;
    tmpfs_fops.lseek64 = (void *)tmpfs_lseek64;
}