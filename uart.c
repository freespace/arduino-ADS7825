#include <stdio.h>

#include <avr/io.h>

#include "uart.h"

/**
 * UART code taken from
 * http://www.appelsiini.net/2011/simple-usart-with-avr-libc with
 * modifications
 */
int uart_putchar(char c, FILE *stream) {
  // wait for data register to be empty
  loop_until_bit_is_set(UCSR0A, UDRE0);
  // send it
  UDR0 = c;
  // Note that this works better than sending a byte, and then blocking until
  // it has been sent. Why? Who knows. Flow control?
  return 0;
}

int uart_getchar(FILE *stream) {
  // wait for data to be available
  loop_until_bit_is_set(UCSR0A, RXC0);

  // read it
  return UDR0;
}

int uart_init(void) {
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |= _BV(U2X0);
#else
  UCSR0A &= ~(_BV(U2X0));
#endif

  // Send 8 bit data
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

  // Enable TX and RX
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);

  FILE *uart_output = fdevopen(uart_putchar, NULL);
  FILE *uart_input = fdevopen(NULL, uart_getchar);

  if (uart_output == NULL || uart_input == NULL) return -1;
  else {
    // this might not be required, since documentation says that the first
    // stream opened with read intent is assigned to stdin, and the first stream
    // opened with write intent is assigned to both stdout and stderr
    stdout = uart_output;
    stdin = uart_input;
    return 0;
  }
}



