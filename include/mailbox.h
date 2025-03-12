#ifndef MAIL_BOX_H
#define MAIL_BOX_H

#define MMIO_BASE       0x3f000000
#define MAILBOX_BASE    (MMIO_BASE + 0xb880)

#define MAILBOX_READ    (volatile unsigned int*)MAILBOX_BASE
#define MAILBOX_STATUS  (volatile unsigned int*)(MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (volatile unsigned int*)(MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY   0x40000000 //31st bit = 1
#define MAILBOX_FULL    0x80000000 //32nd bit = 1

#define CHANNEL_NUMBER 0x8u

int mailbox_entry(void* args);
int mailbox_call(unsigned int mailbox);

void print_board_revision();
void print_arm_memory();


#endif