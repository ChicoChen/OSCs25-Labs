#include "file_sys/initramfs.h"
#include "exception/exception.h"
#include "str_utils.h"
#include "mini_uart.h"

char* newc_magic_str = "070701";
char* terminator = "TRAILER!!!";
int terminator_size = 11;

/*
TODO: restructure initramfs parsing, maybe add a struct to record file structure.
      Which prevent redundent repeated file parsing .
*/
int list_ramfile(void *args){
    if(!initramfs_addr) return 1;

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
        if(buffer[i] == '\n') send_data('\r');
        send_data(buffer[i]);
    }
    // send_data('\n');
    return 0;
}

int view_ramfile(void *args){
    if(!initramfs_addr) return 1;

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
        if(mem[i] == '\n') send_data('\r');
        send_data(mem[i]);
    }
    send_string("\r\n");

    return 0;
}

addr_t find_address(char *filename, unsigned int *filesize_ptr){
    if(!initramfs_addr) return 0;
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

int exec_usr_prog(void* args){
    char *prog_name = "sys_call.img";
    int filesize = 0;
    addr_t source = find_address(prog_name, &filesize);
    addr_t dest = USER_PROG_START;
    for(size_t size = 0; size < filesize; size += 4){ //file content is padded to 4 bytes
        *(uint32_t *)(dest + size) = *(uint32_t *)(source + size);
    }

    _el1_to_el0(USER_PROG_START, USER_STACK_TOP);
    return 0;
}

//----------------------------------------
void set_initramfs_addr(addr_t addr){
    initramfs_addr = (byte *)addr;
}

int check_magic(byte *magic){
    for(int i = 0; i < 6; i++){
        if(magic[i] != newc_magic_str[i]) return 0;
    }
    return 1;
}