#include "file_sys/tmpfs.h"
#include "file_sys/fs_macros.h"
#include "allocator/page_allocator.h"
#include "allocator/dynamic_allocator.h"
#include "utils.h"
#include "str_utils.h"

VnodeOperations tmpfs_vops;
FileOperations tmpfs_fops;

// ----- forward declarations -----
int init_tmpfs_node(TmpfsInternal *node, Vnode *parent, TmpfsType type);
void assign_tmpfs_ops();
int tmpfs_make_childnode(Vnode* parent, Vnode** target, const char* component_name, TmpfsType type);

// ----- public functions -----
void init_tmpfs(){
    tmpfs.name = (char *)dyna_alloc(6);
    memcpy((void *)tmpfs.name, (void *)"tmpfs", 6);
    tmpfs.setup_mount = mount_tmpfs;
    assign_tmpfs_ops();
}

/// @brief setup_mount() of tmpfs
int mount_tmpfs(FileSystem* fs, Mount* mount){
    if(!fs || !mount) return UNKNOWN_ERROR;
    mount->fs = fs;
    mount->root = (Vnode *)dyna_alloc(VNODE_SIZE);
    if(!mount->root) return ALLOCATION_FAILED;
    
    // create internal node for root
    TmpfsInternal *tmpfs_node = (TmpfsInternal*) dyna_alloc(TMPFSINTERNAL_SIZE);
    int internal_init_error = init_tmpfs_node(tmpfs_node, NULL, directory);
    if(internal_init_error) return internal_init_error;

    int v_init_error = init_vnode(mount->root, mount, (void *)tmpfs_node, &tmpfs_vops, &tmpfs_fops);
    if(v_init_error) return v_init_error;
    return 0;
}

// ----- vnode operations -----
int tmpfs_lookup_i(Vnode* dir_node, Vnode** target, const char* component_name){
	*target = NULL;
    TmpfsInternal *internal = (TmpfsInternal *)dir_node->internal;
	for(size_t i = 0; i < internal->num_children; i++){
        if(strcmp(component_name, ".")){
            *target = dir_node;
            return 0;
        }
        if(strcmp(component_name, "..")){
            *target = internal->parent;
        }
        else if(!strcmp(internal->children_name[i], component_name)) continue;
        *target = internal->children[i];
    }

	return (*target)? 0: FILE_NOT_FOUND;
}

int tmpfs_create_i(Vnode* dir_node, Vnode** target, const char* component_name){
    return tmpfs_make_childnode(dir_node, target, component_name, content_file);
}

int tmpfs_mkdir_i(Vnode* dir_node, Vnode** target, const char* component_name){
    return tmpfs_make_childnode(dir_node, target, component_name, directory);
}

// ----- file operations -----
int tmpfs_write_i(FileHandler* file, const void* buf, size_t len){

}

int tmpfs_read_i(FileHandler* file, void* buf, size_t len){

}

int tmpfs_open_i(Vnode* file_node, FileHandler** target){
    FileHandler *new_fd = (FileHandler *)dyna_alloc(FILEHANDLER_SIZE);
    if(!new_fd) return ALLOCATION_FAILED;

    new_fd->vnode = file_node;
    new_fd->f_pos = 0;
    new_fd->ops = &tmpfs_fops;
    *target = new_fd;
    return 0;
}

int tmpfs_close_i(FileHandler* file){

}

long tmpfs_lseek64(FileHandler* file, long offset, int whence){

}


// ----- local methods -----
int init_tmpfs_node(TmpfsInternal *node, Vnode *parent, TmpfsType type){
    node->type = type;
    node->parent = parent;
    node->num_children = 0;
    node->children_name = (char **)dyna_alloc(8 * MAX_FILE_ENTRY_SIZE);
    if(!node->children_name) return ALLOCATION_FAILED;

    
    for(size_t i = 0; i < MAX_FILE_ENTRY_SIZE; i++) node->children[i] = NULL;
    node->file_size = 0;
    if(type == directory) node->content = NULL;
    else {
        node->content = page_alloc(FILE_CONTENT_LENGTH);
        if(!node->content) return ALLOCATION_FAILED;
    }
    return 0;
}

void assign_tmpfs_ops(){
    // vops
    

    // fops
    tmpfs_fops.open = (void *)tmpfs_open_i;
    tmpfs_fops.read = (void *)tmpfs_read_i;
    tmpfs_fops.write = (void *)tmpfs_write_i;
    tmpfs_fops.close = (void *)tmpfs_close_i;
    tmpfs_fops.lseek64 = (void *)tmpfs_lseek64;
}

int tmpfs_make_childnode(Vnode* parent, Vnode** target, const char* component_name, TmpfsType type){
    *target = NULL;
    TmpfsInternal *par_internal = parent->internal;
    if(par_internal->num_children == MAX_FILE_ENTRY_SIZE) return OPERATION_NOT_ALLOW;
    size_t idx = par_internal->num_children;

    // pathname
    par_internal->children_name[idx] = (char *)dyna_alloc(FILE_NAME_LENGTH);
    if(!par_internal->children_name[idx]) return ALLOCATION_FAILED;
    memcpy((void *)par_internal->children_name[idx], (void *)component_name, get_size(component_name));

    // child node
    TmpfsInternal *child_internal = (TmpfsInternal *)dyna_alloc(TMPFSINTERNAL_SIZE);
    if(!child_internal) return ALLOCATION_FAILED;

    init_tmpfs_node(child_internal, parent, type);
    *target = (Vnode *)dyna_alloc(VNODE_SIZE);
    if(!*target) return ALLOCATION_FAILED;
    init_vnode(*target, NULL, child_internal, &tmpfs_vops, &tmpfs_fops);

    par_internal->children[idx] = *target;
    par_internal->num_children++;
    return 0;
}