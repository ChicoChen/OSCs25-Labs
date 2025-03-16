#ifndef MINI_UART_H
#define MINI_UART_H
#include "base_address.h"

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

void init_uart();

char read_data();
int echo_read_line(char *inputline);

void send_data(char c);
void send_string(char *str);
void send_line(char *line);

#endif