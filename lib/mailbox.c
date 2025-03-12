#include "mailbox.h"
#include "mini_uart.h"
#include "str_utils.h"

#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005

#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

int mailbox_entry(void* args){
    send_line("Mailbox info:\r\n");
    print_board_revision();
    send_line("\r\n");
    print_arm_memory();
    send_line("\r\n");
    return 0;
}

void print_board_revision(){
    unsigned int __attribute__((aligned(16))) mailbox[7];
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = END_TAG;
    
    mailbox_call((unsigned int)mailbox);
    char board_revision[11];
    send_line(" Board revision: ");
    send_line(itoa(mailbox[5], board_revision, HEX)); // it should be 0xa020d3 for rpi3 b+
}

void print_arm_memory(){
    unsigned int __attribute__((aligned(16))) mailbox[8];

    mailbox[0] = 8 * 4;
    mailbox[1] = REQUEST_CODE;
    mailbox[2] = GET_ARM_MEMORY;
    mailbox[3] = 8;
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;
    mailbox[6] = 0;
    mailbox[7] = END_TAG;
    
    mailbox_call((unsigned int)mailbox);
    char mem_bass_addr[11];
    send_line(" ARM memory base address: ");
    send_line(itoa(mailbox[5], mem_bass_addr, HEX));
    send_line("\r\n ARM memory size: ");
    send_line(itoa(mailbox[6], mem_bass_addr, HEX));
}

int mailbox_call(unsigned int mailbox){
    unsigned int message = (mailbox & ~(0xf)) | CHANNEL_NUMBER;
    unsigned recv = 0x0u;
    while(message != recv){
        while((*MAILBOX_STATUS) & MAILBOX_FULL); //wait until no full
        *MAILBOX_WRITE = message;
        
        while(*MAILBOX_STATUS & MAILBOX_EMPTY); //wait until not empty
        recv = *MAILBOX_READ;
    }
    return 1;
}