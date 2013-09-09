#include <stdio.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include "uart.h"

#include "ads7825.h"

int main(void) {
  uart_init();
  adc_init();
  
  printf("Ready\n");
  adc_read_analog(0);
  printf("Primed\n");

  for (;;) {
    printf("%6d %6d %6d %6d\n",
        adc_read_analog(1),
        adc_read_analog(2),
        adc_read_analog(3),
        adc_read_analog(0));
  }

  return 0;
}
