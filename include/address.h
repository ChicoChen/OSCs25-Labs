#ifndef ADDRESS_H
#define ADDRESS_H

#define MMIO_BASE   0x3F000000
#define GPIO_BASE   MMIO_BASE + 0x200000

//----- Mini-uart -----
#define GPFSEL1         (volatile unsigned int*)(GPIO_BASE + 0x00004)
#define GPPUD           (volatile unsigned int*)(GPIO_BASE + 0x00094)
#define GPPUD_CLK0      (volatile unsigned int*)(GPIO_BASE + 0x00098)

#define AUXENB          (volatile unsigned int*)(GPIO_BASE + 0x15004)
#define AUX_MU_CNTL_REG (volatile unsigned int*)(GPIO_BASE + 0x15060)
#define AUX_MU_IER_REG  (volatile unsigned int*)(GPIO_BASE + 0x15044)
#define AUX_MU_LCR_REG  (volatile unsigned int*)(GPIO_BASE + 0x1504C)
#define AUX_MU_MCR_REG  (volatile unsigned int*)(GPIO_BASE + 0x15050)
#define AUX_MU_BAUD_REG (volatile unsigned int*)(GPIO_BASE + 0x15068)
#define AUX_MU_IIR_REG  (volatile unsigned int*)(GPIO_BASE + 0x15048)
#define AUX_MU_CNTL_REG (volatile unsigned int*)(GPIO_BASE + 0x15060)

#define AUX_MU_LSR_REG  (volatile unsigned int*)(GPIO_BASE + 0x15054)
#define AUX_MU_IO_REG   (volatile unsigned int*)(GPIO_BASE + 0x15040)

//----- Mailbox -----
#define MAILBOX_BASE    MMIO_BASE + 0xb880

#define MAILBOX_READ    (volatile unsigned int*)(MAILBOX_BASE)
#define MAILBOX_STATUS  (volatile unsigned int*)(MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (volatile unsigned int*)(MAILBOX_BASE + 0x20)

//----- Reboot -----


#endif