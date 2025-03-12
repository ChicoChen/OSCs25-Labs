#ifndef MINI_UART_H
#define MINI_UART_H

#include "address.h"

void init_uart();

char read_data();

void send_data(char c);
void send_line(char *line);

#endif