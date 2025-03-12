#ifndef MINI_UART_H
#define MINI_UART_H

#define GPFSEL1         (volatile unsigned int*)0x3F200004 //map from 7E to 3F
#define GPPUD           (volatile unsigned int*)0x3F200094
#define GPPUD_CLK0      (volatile unsigned int*)0x3F200098

#define AUXENB          (volatile unsigned int*)0x3F215004
#define AUX_MU_CNTL_REG (volatile unsigned int*)0x3F215060
#define AUX_MU_IER_REG  (volatile unsigned int*)0x3F215044
#define AUX_MU_LCR_REG  (volatile unsigned int*)0x3F21504C
#define AUX_MU_MCR_REG  (volatile unsigned int*)0x3F215050
#define AUX_MU_BAUD_REG (volatile unsigned int*)0x3F215068
#define AUX_MU_IIR_REG  (volatile unsigned int*)0x3F215048
#define AUX_MU_CNTL_REG (volatile unsigned int*)0x3F215060

#define AUX_MU_LSR_REG  (volatile unsigned int*)0x3F215054
#define AUX_MU_IO_REG  (volatile unsigned int*)0x3F215040

void init_uart();

char read_data();

void send_data(char c);
void send_line(char *line);

#endif