#include "file_sys/initramfs.h"
#include "file_sys/vfs.h"
#include "file_sys/fs_macros.h"
#include "devicetree/dtb.h"
#include "exception/exception.h"
#include "thread/thread.h"
#include "allocator/dynamic_allocator.h"
#include "memory_region.h"
#include "base_address.h"
#include "utils.h"
#include "str_utils.h"
#include "mini_uart.h"

FileSystem initramfs;
Vnode initramfs_root;

void *initramfs_addr = NULL;
size_t initramfs_size = 0;
char* newc_magic_str = "070701";
char* terminator = "TRAILER!!!";
int terminator_size = 11;

// ----- forward declaration -----
void init_initramfs_internal(InitramfsInternal *target, InitramfsType type,
    Vnode *parent);

addr_t find_address(char *filename, unsigned int *filesize_ptr);
void set_initramfs(unsigned int type, char *name, void *data, size_t len);
size_t get_ramfs_size();
int check_magic(byte* magic);

int parse_cpio();
void *get_next_initramfs_node();
// ----- public interface -----
// todo 1: parse initramfs and construct file tree
// todo 2: replace all implemented function with on-tree version

void get_initramfs_info(){
    dtb_parser(set_initramfs, (addr_t)_dtb_addr);
    get_ramfs_size();
}

void init_initramfs(){
    initramfs.name = (char *)dyna_alloc(10);
    memcpy(initramfs.name, "initramfs", 10);
    initramfs.setup_mount = mount_initramfs;
    // assign_initramfs_ops();

    // parse_cpio();
}

int mount_initramfs(FileSystem *fs, Mount *mount){
    if(!fs || !mount) return UNKNOWN_ERROR;
    mount->fs = fs;
    mount->root = &initramfs_root; // todo: maybe mount_i is a better choice

    InitramfsInternal *root_node = (InitramfsInternal *)dyna_alloc(sizeof(InitramfsInternal));

}

int initramfs_lookup_i(Vnode *dir_node, Vnode **target, const char *component_name){
    InitramfsInternal *internal = (InitramfsInternal *)dir_node->internal;
    if(internal->type != directory) return OPERATION_NOT_ALLOW;

    InitramfsChild *child = internal->data.children;
    while(child){
        if(strcmp(component_name, child->name)){
            *target = child->vnode;
            return 0;
        }
        
        child = (child->list_node.next)?
                GET_CONTAINER(child->list_node.next, InitramfsChild, list_node):
                NULL;
    }
    return FILE_NOT_FOUND;
}

int initramfs_create_i(Vnode *dir_node, Vnode **target, const char *component_name){
    return OPERATION_NOT_ALLOW;
}

int initramfs_mkdir_i(Vnode *dir_node, Vnode **target, const char *component_name){
    return OPERATION_NOT_ALLOW;
}

int initramfs_open_i(Vnode* file_node, FileHandler** target){

}

int initramfs_read_i(FileHandler* file, void* buf, size_t len){

}

int initramfs_write_i(FileHandler* file, const void* buf, size_t len){
    return OPERATION_NOT_ALLOW;
}

int initramfs_close_i(FileHandler* file){

}

int list_ramfile(void *args){
    if(!initramfs_addr) get_initramfs_info();

    char buffer[LS_BUFFER_SIZE];
    byte *mem = initramfs_addr;
    int writehead = 0;
    while(1){
        CpioNewcHeader *header = (CpioNewcHeader*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int filesize = carrtoi(header->c_filesize, 8, HEX);
        int pathsize = carrtoi(header->c_namesize, 8, HEX);

        mem += CPIO_HEADER_SIZE;
        for(int i = 0 ; i < pathsize; i++){
            buffer[writehead++] = mem[i];
        }
        if(strcmp(mem, terminator)) break;
        buffer[writehead++] = '\n';

        mem += pathsize;
        while(((unsigned int) mem) % 4) mem++;

        mem += filesize;
        while(((unsigned int) mem) % 4) mem++;
    }

    for(int i = 0; i < writehead - terminator_size; i++){
        if(buffer[i] == '\n') async_send_data('\r');
        async_send_data(buffer[i]);
    }
    // send_data('\n');
    return 0;
}

int view_ramfile(void *args){
    if(!initramfs_addr) get_initramfs_info();

    char *filename = *(char**) args;
    if(filename == NULL) return 1;

    byte *mem = initramfs_addr;
    char buffer[CAT_BUFFER_SIZE];
    int writehead = 0;
    int filesize = 0;
    
    int found = 0;
    while(1){
        CpioNewcHeader *header = (CpioNewcHeader*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int pathsize = carrtoi(header->c_namesize, 8, HEX);
        filesize = carrtoi(header->c_filesize, 8, HEX);

        mem += CPIO_HEADER_SIZE;
        if(strcmp(mem, terminator)) return -1;
        else if(strcmp(mem, filename)) found = 1;

        mem += pathsize;
        while(((unsigned int) mem) % 4) mem++;
        if(found) break;

        mem += filesize;
        while(((unsigned int) mem) % 4) mem++;
    }

    for(int i = 0; i < filesize; i++){
        if(mem[i] == '\n') async_send_data('\r');
        async_send_data(mem[i]);
    }
    send_string("\r\n");
    
    char temp[20];
    send_string(itoa(filesize, temp, DEC));
    send_string("filesize: ");
    send_line(temp);
    return 0;
}

RCregion *load_program(char *prog_name){
    if(!prog_name) prog_name = "sys_call.img";
    size_t filesize = 0;
    addr_t source = find_address(prog_name, &filesize);
    if(!source) {
        send_line("[ERROR][filesys]: can't find target program!");
        return NULL;
    }
    
    RCregion *dest = rc_alloc(filesize);
    if(!dest){
        send_line("[ERROR][filesys]: can't allocate space for program!");
        return NULL;
    }
    
    char temp[32];
    send_string("[filesys]: loadling program of size ");
    send_line(itoa(filesize, temp, HEX));
    memcpy(dest->mem, (void *)source, filesize);
    return dest;
}

/// @brief spawn a new thread to run specific program in EL0, used only by kernel's simple_shell
/// @param name name of program.
/// @param args needed arguments
int run_prog(char *prog_name, char **args){
    // TODO: how to run with arguments?
    RCregion *dest = load_program(prog_name);
    if(!dest) return 1;
    
    create_prog_thread(dest);
    schedule();
    return 0;
}

// ----- private members -----
int parse_cpio(){
    if(!initramfs_addr) get_initramfs_info();
    
    byte *read_head = (byte *)initramfs_addr;
    char path[INITRAMFS_MAX_PATH_LEN];
    while(true){
        CpioNewcHeader *header = (CpioNewcHeader *)read_head;
        if(!check_magic(header->c_magic)) return -1;

        int pathsize = carrtoi(header->c_namesize, 8, HEX);
        int filesize = carrtoi(header->c_filesize, 8, HEX);
        if(pathsize + 1 >= INITRAMFS_MAX_PATH_LEN) return ALLOCATION_FAILED;

        read_head += CPIO_HEADER_SIZE;
        memcpy((void *)path, (void *)read_head, pathsize);
        path[pathsize] = '\0'; // null terminate
        // todo:
        // 1. parse path
        char *tok = strtok(path, "/");
        Vnode *curr_node = &initramfs_root;
        Vnode *next_node = NULL;
        while(tok){
            // child of initramfs node must be cpio node.
            int error = initramfs_lookup_i(curr_node, &next_node, tok);
            if(error == FILE_NOT_FOUND) {
                // create new node;
            }
            else if(error) return error;

            curr_node = next_node;
            tok = strtok(NULL, "/");
        }
        
        // 2. copy content
        read_head = (byte *)align((void *)(read_head + pathsize), 4);
        byte *content = read_head;
        
        // 3. decide is it a file or path
        InitramfsType type = (filesize == 0)? directory: content_file;

        // 4. create and init node
    }

}

void init_initramfs_internal(InitramfsInternal *target, InitramfsType type,
    Vnode *parent)
{
    target->type = type;
    target->parent = parent;

    if(type == directory){
        target->data_size.num_children = 0;
        target->data.children = NULL;
    }
    else{
        target->data_size.filesize = 0;
        target->data.file_content = NULL;
    }
}

int create_child(Vnode *parent, Vnode **target, const char *child_name, InitramfsType child_type){
    InitramfsInternal *parent_internal = (InitramfsInternal *)parent->internal;
    if(parent_internal->type != directory) return OPERATION_NOT_ALLOW;
    
    InitramfsChild *child = (InitramfsChild *)dyna_alloc(sizeof(InitramfsChild));
    node_init(&child->list_node);
    
    // assign filename
    size_t name_len = get_size(child_name);
    child->name = (char *)dyna_alloc(name_len);
    memcpy(child->name, child_name, name_len);
    
    // create vnode
    child->vnode = dyna_alloc(sizeof(Vnode));
    InitramfsInternal *child_internal = dyna_alloc(sizeof(InitramfsInternal));
    init_initramfs_internal(child_internal, child_type, parent);
    init_vnode(child->vnode, NULL, child_internal, &initramfs_vops, &initramfs_fops);

    if(parent_internal->data_size.num_children == 0){
        parent_internal->data.children = child;
        return 0;
    }

    // for non-empty directory
    InitramfsChild *curr = parent_internal->data.children;
    while(true){
        if(strcmp(curr->name, child_name)) {
            // todo: reclaim memory
            return OPERATION_NOT_ALLOW;
        }
        else if(!curr->list_node.next) break;
        curr = GET_CONTAINER(curr->list_node.next, InitramfsChild, list_node);
    }
    list_add(&child->list_node, curr, NULL);
    return 0;
}

addr_t find_address(char *filename, unsigned int *filesize_ptr){
    if(!initramfs_addr) get_initramfs_info();
    else if(filename == NULL) return 0;

    byte *mem = initramfs_addr;
    int found = 0;
    while(1){
        CpioNewcHeader *header = (CpioNewcHeader*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int pathsize = carrtoi(header->c_namesize, 8, HEX);
        *filesize_ptr = carrtoi(header->c_filesize, 8, HEX);

        mem += CPIO_HEADER_SIZE;
        if(strcmp(mem, terminator)) return 0;
        else if(strcmp(mem, filename)) found = 1;

        mem += pathsize;
        while(((unsigned int) mem) % 4) mem++;
        if(found) break;

        mem += *filesize_ptr;
        while(((unsigned int) mem) % 4) mem++;
    }

    return (addr_t)mem;
}

/// @brief callback func provide to dtb_parser to find and set address of initramfs
/// @param type Token type of this data in dtb (should be property)
/// @param name name of this property
/// @param data big_endien interpret of address
/// @param len 
void set_initramfs(unsigned int type, char *name, void *data, size_t len){
    if(type == FDT_PROP && strcmp("linux,initrd-start", name)){
        unsigned int cpio_addr = to_le_u32(*(unsigned int*)data);
        send_string("initramfs address found: ");
        char addr[11];
        send_line(itoa(cpio_addr, addr, HEX));
        initramfs_addr = (void *)cpio_addr;
    }
}

size_t get_ramfs_size(){
    if(!initramfs_addr) get_initramfs_info();

    byte *mem = initramfs_addr;
    while(1){
        CpioNewcHeader *header = (CpioNewcHeader*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int filesize = carrtoi(header->c_filesize, 8, HEX);
        int pathsize = carrtoi(header->c_namesize, 8, HEX);

        mem += CPIO_HEADER_SIZE;
        if(strcmp(mem, terminator)) {
            mem += pathsize;
            while(((unsigned int) mem) % 4) mem++;
            break;
        }

        mem += pathsize;
        while(((unsigned int) mem) % 4) mem++;

        mem += filesize;
        while(((unsigned int) mem) % 4) mem++;
    }
    
    initramfs_size = (size_t)mem - (size_t)initramfs_addr;
    char size_arr[16];
    send_string("initramfs size: ");
    send_line(itoa(initramfs_size, size_arr, HEX));
    return initramfs_size;
}

int check_magic(byte *magic){
    for(int i = 0; i < 6; i++){
        if(magic[i] != newc_magic_str[i]) return 0;
    }
    return 1;
}