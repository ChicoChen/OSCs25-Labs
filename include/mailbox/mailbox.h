#ifndef MAIL_BOX_H
#define MAIL_BOX_H

#include "../base_address.h"
#define MAILBOX_READ    (volatile unsigned int*)(MAILBOX_BASE)
#define MAILBOX_STATUS  (volatile unsigned int*)(MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (volatile unsigned int*)(MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY   0x40000000 //31st bit = 1
#define MAILBOX_FULL    0x80000000 //32nd bit = 1
#define CHANNEL_NUMBER 0x8u

int mailbox_call(char ch, unsigned int mailbox_addr);

void print_board_revision();
void print_arm_memory();


#endif