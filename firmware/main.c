#include <stdio.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>

#include "uart.h"

#include "ads7825.h"

#define BUFFER_SIZE               (512)

void timer_init(uint8_t top) {
  // setup counter0 issue an interrupt every time its value reaches top

  // OC0A and OC0B disconnected, normal waveform generation mode
  TCCR0A = 0x00 | _BV(WGM01);

  // CTC waveform generation mode, clearing the timer on match
  TCCR0B = 0x00 | _BV(CS02) | _BV(CS01);

  // set the value at which we roll over, using OCR0A
  OCR0A = top;

  // issue an interrupt when OCR0A matches TCNT0
  TIMSK0 = 0x00 | _BV(OCIE0A);

  TCNT0 = 0;

  sei();
}


typedef struct {
  int16_t data[BUFFER_SIZE];
  uint16_t writepos;
} Buffer;

volatile Buffer buffer;

ISR(TIMER0_COMPA_vect) {
  if (buffer.writepos < BUFFER_SIZE) {
    buffer.data[buffer.writepos++] = adc_read_analog(1);
    buffer.data[buffer.writepos++] = adc_read_analog(2);
    buffer.data[buffer.writepos++] = adc_read_analog(3);
    buffer.data[buffer.writepos++] = adc_read_analog(0);
  }
}

int main(void) {
  uart_init();
  adc_init();
  timer_init(10);

  // prime the next channel to be read as channel 0
  adc_read_analog(0);
  
  printf("Ready\n");

  for (;;) {
    int c = getchar();
    if (c == 'r') {
      // prime channel 0 for read, just in case, even though we should always
      // end with channel 0 primed for next read
      adc_read_analog(0);

      // we don't do this inside a printf statement because we need the reads
      // to occur in a specific order, and function argument evaluation order
      // is not guaranteed
      int chan0 = adc_read_analog(1);
      int chan1 = adc_read_analog(2);
      int chan2 = adc_read_analog(3);
      int chan3 = adc_read_analog(0);

      printf("%6d, %6d, %6d, %6d\n",
          chan0,
          chan1,
          chan2,
          chan3);
    } else if (c == 's') {
      // disable interrupts briefly so we can be sure no one is touching
      // buffer
      cli();

      // reset to start of the buffer
      buffer.writepos = 0;

      // prime channel 0 for read, just in case, even though we should always
      // end with channel 0 primed for next read
      adc_read_analog(0);

      // set the counter to 0 so we don't trigger prematurely in case TCNT0
      // has left over values
      TCNT0 = 0;

      // re-enable interrupts
      sei();
    } else if (c == 'p') {
      int idx;
      for (idx = 0; idx < buffer.writepos; idx += 4) {
      printf("%d %d %d %d %d\n",
          idx,
          buffer.data[idx+0],
          buffer.data[idx+1],
          buffer.data[idx+2],
          buffer.data[idx+3]);
      }
      printf("END\n");
    }
  }

  return 0;
}
