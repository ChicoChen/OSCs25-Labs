#include "file_sys/tmpfs.h"
#include "allocator/dynamic_allocator.h"
#include "utils.h"

VnodeOperations tmpfs_vops;
FileOperations tmpfs_fops;

void init_tmpfs(){
    tmpfs.name = (char *)dyna_alloc(6);
    memcpy(tmpfs.name, "tmpfs", 6);
    tmpfs.setup_mount = mount_tmpfs;
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