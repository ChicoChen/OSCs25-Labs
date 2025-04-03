#ifndef MINI_UART_H
#define MINI_UART_H

#include "base_address.h"
#include "basic_type.h"

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

// interrupt related
#define ENABLE_IRQs1    (volatile unsigned int*)(ARM_INTERRUPT_BASE + 0x210)
#define DISABLE_IRQs1    (volatile unsigned int*)(ARM_INTERRUPT_BASE + 0x21C)
#define AUXIRQ          (volatile unsigned int*)(GPIO_BASE + 0x15000)

#define ASYNC_BUFFER_SIZE 256u

typedef struct{
    byte buffer[ASYNC_BUFFER_SIZE];
    size_t head;
    size_t len;
}AsyncBuf;

void disable_aux_interrupt();
void enable_aux_interrupt();

static AsyncBuf async_recv;
static AsyncBuf async_tran;

void init_uart();
// ----- read -----
char sync_read_data();
char async_read_data();

int echo_read_line(char *inputline);

// ----- write -----
void sync_send_data(char c);
void async_send_data(char c);

void send_string(char *str); //default async
void _send_string_(char *str, void (*send_func)(char));

void send_line(char *line);
void send_void_line(void *vstr);
void _send_line_(char *line, void (*send_func)(char));

// ----- exceptions -----
void uart_except_handler();
#endif