#include "file_sys/initramfs.h"
#include "str_utils.h"
#include "mini_uart.h"

char* newc_magic_str = "070701";
char* terminator = "TRAILER!!!";
int terminator_size = 11;

int list_file(void *args){
    char buffer[LS_BUFFER_SIZE];
    char *mem = (char *)INITRAMFS_ADDRESS;
    int write_head = 0;
    while(1){
        cpio_newc_header *header = (cpio_newc_header*)mem;
        if(!check_magic(header->c_magic)) return 1;
        int filesize = carrtoi(header->c_filesize, 8, HEX);
        int pathsize = carrtoi(header->c_namesize, 8, HEX);

        mem += HEADER_SIZE;
        for(int i = 0 ; i < pathsize; i++){
            buffer[write_head++] = mem[i];
        }
        if(strcmp(mem, terminator)) break;
        buffer[write_head++] = '\n';

        mem += pathsize;
        while(((unsigned int) mem) % 4) mem++;

        mem += filesize;
        while(((unsigned int) mem) % 4) mem++;
    }

    for(int i = 0; i < write_head - terminator_size; i++){
        send_data(buffer[i]);
    }
    // send_data('\n');
    return 0;
}


int check_magic(char *magic){
    for(int i = 0; i < 6; i++){
        if(magic[i] != newc_magic_str[i]) return 0;
    }
    return 1;
}