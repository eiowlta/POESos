#ifndef UART_H
#define UART_H
#include "eos.h"
void uart_init();
//void uart_sendbyte(tByte c);

void uart_write(char *);
byte uart_read(char *, byte max);
void uart_writei(unsigned int i);
#endif // UART_H

