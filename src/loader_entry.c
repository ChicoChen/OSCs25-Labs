#include "mini_uart.h"
#include "str_utils.h"
#define KERNEL_ADDR (unsigned int *)(0x80000)
#define HEADER_SIZE 8
#define FLAG_SIZE 4

int loader_entry(){
    init_uart();
    _send_line_("waiting for protocal header...", sync_send_data);
    char header[HEADER_SIZE];
    
    for(int header_idx = 0; header_idx < HEADER_SIZE; header_idx++){
        char c = sync_read_data();
        sync_send_data(c);
        header[header_idx] = c;
    }

    int kernel_size = 0;
    for(int i = FLAG_SIZE; i < HEADER_SIZE; i++){
        kernel_size *= 256; // 1 byte = 256
        kernel_size += (unsigned int)header[i];
    }
    char size[9];
    _send_line_(itoa(kernel_size, size, DEC), sync_send_data);

    _send_line_("waiting for kernel image...", sync_send_data);
    unsigned int *target_addr = KERNEL_ADDR;
    for(int received = 0; received < kernel_size; received+= 4){
        unsigned int word = 0;
        for(int byte = 0; byte < 4; byte++){
            word += sync_read_data() << 8 * byte; //little endian
        }
        *target_addr++ = word;
    }
    return 0;
}