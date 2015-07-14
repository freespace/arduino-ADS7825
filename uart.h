#ifndef UART_H
#define UART_H

#define F_CPU               (16000000UL)
#define BAUD                (250000UL)
#include <util/setbaud.h>

int uart_init(void);
int uart_putchar(char c, FILE *stream);

#endif
