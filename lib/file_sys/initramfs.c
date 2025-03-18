#include "file_sys/initramfs.h"
#include "str_utils.h"
#include "mini_uart.h"

char* newc_magic_str = "070701";
char* terminator = "TRAILER!!!";
int terminator_size = 11;

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

    send_string("Finename: ");
    char filename[MAX_FILENAME];
    echo_read_line(filename);

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

void set_initramfs_addr(addr_t addr){
    initramfs_addr = (byte *)addr;
}

int check_magic(byte *magic){
    for(int i = 0; i < 6; i++){
        if(magic[i] != newc_magic_str[i]) return 0;
    }
    return 1;
}