#include "file_sys/initramfs.h"
#include "devicetree/dtb.h"
#include "exception/exception.h"
#include "memory_region.h"
#include "base_address.h"
#include "utils.h"
#include "str_utils.h"
#include "mini_uart.h"

void *initramfs_addr = NULL;
size_t initramfs_size = 0;
char* newc_magic_str = "070701";
char* terminator = "TRAILER!!!";
int terminator_size = 11;

// ----- forward declaration -----
void set_initramfs(unsigned int type, char *name, void *data, size_t len);
size_t get_ramfs_size();
// ----- public interface -----
/*
TODO: restructure initramfs parsing, maybe add a struct to record file structure.
      Which prevent redundent repeated file parsing .
*/

void init_ramfile(){
    dtb_parser(set_initramfs, (addr_t)_dtb_addr);
    get_ramfs_size();
}

int list_ramfile(void *args){
    if(!initramfs_addr) init_ramfile();

    char buffer[LS_BUFFER_SIZE];
    byte *mem = initramfs_addr;
    int writehead = 0;
    while(1){
        cpio_newc_header *header = (cpio_newc_header*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int filesize = carrtoi(header->c_filesize, 8, HEX);
        int pathsize = carrtoi(header->c_namesize, 8, HEX);

        mem += HEADER_SIZE;
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
    if(!initramfs_addr) init_ramfile();

    char *filename = *(char**) args;
    if(filename == NULL) return 1;

    byte *mem = initramfs_addr;
    char buffer[CAT_BUFFER_SIZE];
    int writehead = 0;
    int filesize = 0;
    
    int found = 0;
    while(1){
        cpio_newc_header *header = (cpio_newc_header*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int pathsize = carrtoi(header->c_namesize, 8, HEX);
        filesize = carrtoi(header->c_filesize, 8, HEX);

        mem += HEADER_SIZE;
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

    return 0;
}

addr_t find_address(char *filename, unsigned int *filesize_ptr){
    if(!initramfs_addr) init_ramfile();
    else if(filename == NULL) return 0;

    byte *mem = initramfs_addr;
    int found = 0;
    while(1){
        cpio_newc_header *header = (cpio_newc_header*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int pathsize = carrtoi(header->c_namesize, 8, HEX);
        *filesize_ptr = carrtoi(header->c_filesize, 8, HEX);

        mem += HEADER_SIZE;
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

int exec_user_prog(char *prog_name, char **args){
    //TODO: how to run with arguments?
    if(!prog_name) prog_name = "sys_call.img";
    size_t filesize = 0;
    addr_t source = find_address(prog_name, &filesize);
    if(!source) {
        send_line("[ERROR][filesys]: can't find target program!");
        return 1;
    }
    
    addr_t dest = USER_PROG_START;
    for(size_t size = 0; size < filesize; size += 4){ //file content is padded to 4 bytes
        *(uint32_t *)(dest + size) = *(uint32_t *)(source + size);
    }

    _el1_to_el0(USER_PROG_START, USER_STACK_TOP);
    return 0;
}

// ----- private members -----

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
    if(!initramfs_addr) init_ramfile();

    byte *mem = initramfs_addr;
    while(1){
        cpio_newc_header *header = (cpio_newc_header*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int filesize = carrtoi(header->c_filesize, 8, HEX);
        int pathsize = carrtoi(header->c_namesize, 8, HEX);

        mem += HEADER_SIZE;
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