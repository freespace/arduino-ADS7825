#include <stdio.h>
#include <math.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>

#include "uart.h"

#include "ads7825.h"

// Make room for 512 scans of each of 4 channels
#define BUFFER_SIZE               (512 * 4)

void timer_init(uint8_t n) {
  // setup counter0 issue an interrupt for every n external trigger seen.

  // OC0A and OC0B disconnected
  // CTC waveform generation mode, clearing the timer on match
  TCCR0A = 0x00 | _BV(WGM01);

  // Set T0 as external clock, clock on falling edge
  TCCR0B = 0x00 | _BV(CS02) | _BV(CS01);

  // set the value at which we roll over, using OCR0A. We enforce the
  // condition that n is >=1 because triggering for every 0 external trigger
  // seen makes no sense.
  n = fmaxf(1, n);

  // the -1 is because TCNT0 goes from top-1, top*, 0, 1, ..., top-1, ...  with
  // interrupts occuring on *. Thus to have an interrupt for every
  // other trigger (n=2), you need OCR0A=1, and TCNT0 will go 0, 1*, 0, 1*
  OCR0A = n-1;

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
    // doing buffer.data[buffer.writepos++] = ... is much slower than
    // doing what is done now. This makes things so slow we can't keep
    // up with what is a relatively slow external clock! This is much faster.
    // Will need to investigate why if time permits. Hypothesis: 16bit
    // increments are very slow, and cheaper to do one increment by 4 then 4
    // increment by ones due to lack of dedicate instructions for 16bit
    // increments.
    //
    // Note that the arduino implementation uses x++ pattern, and may be the
    // cause of its slowness, and why I needed an external frequency divider
    // to get it to work reliably.
    buffer.data[buffer.writepos+0] = adc_read_analog(1);
    buffer.data[buffer.writepos+1] = adc_read_analog(2);
    buffer.data[buffer.writepos+2] = adc_read_analog(3);
    buffer.data[buffer.writepos+3] = adc_read_analog(0);
    buffer.writepos+=4;
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

    // disable interrupts because we don't want the buffer being written
    // to for 'p' or 'b', nor writepos being updated
    cli();

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
      // reset to start of the buffer
      buffer.writepos = 0;

      // prime channel 0 for read, just in case, even though we should always
      // end with channel 0 primed for next read
      adc_read_analog(0);

      // set the counter to 0xFF so we don't trigger prematurely in case TCNT0
      // has left over values
      //
      // XXX why 0xFF and not 0? Because the first trigger needs to set TCNT0
      // to 0. If we want every to do a scan every other trigger, then
      // OCR0A=1. Thus if we start with TCNT0 = 0, the first trigger that arrives
      // will increment TCNT0 to 1, which is a match, and we immediately get
      // an interrupt. On the next trigger TCNT0 = 0, then after that TCNT0=1,
      // and another interrupt happens.
      TCNT0 = 0xFF;

      PORTB = 0;

    } else if (c == 'p') {
      int idx;
      for (idx = 0; idx < buffer.writepos; idx += 4) {
      printf("%d %d %d %d %d\n",
          idx/4,
          buffer.data[idx+0],
          buffer.data[idx+1],
          buffer.data[idx+2],
          buffer.data[idx+3]);
      }
      printf("END\n");
    } else if (c == 'b') {
      // sends the buffer in binary. The first 2 bytes indicate the number of
      // bytes to follow. All data are in fact 16bit integers, sent LSB first
      // since AVRs are little endian machines
      fwrite((void *)&buffer.writepos, sizeof buffer.writepos, 1, stdout);
      fwrite((void *)buffer.data, sizeof buffer.data[0], buffer.writepos, stdout);
    } else if (c == 'c') {
      // send the number of readings in the buffer. Each reading is from a
      // channel, and is a 16bit integer
      fwrite((void *)&buffer.writepos, sizeof buffer.writepos, 1, stdout);
    }

    // re-enable interrupts after processing serial commands
    sei();
  }

  return 0;
}
