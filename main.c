#include <stdio.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "errors.h"
#include "ads7825.h"
#include "digital_out.h"
#include "trigger.h"


#define CHANNELS_AVAILABLE    (4)
uint8_t nchannels = CHANNELS_AVAILABLE;

// keep in mind that each element of the buffer is 16bits, so takes up two
// bytes
#define BUFFER_SIZE               (2048+1024)

typedef struct {
  int16_t data[BUFFER_SIZE];
  uint16_t writepos;
} Buffer;

volatile Buffer buffer;

ISR(TIMER0_OVF_vect) {
  PORTB = _BV(PB7);
  if (buffer.writepos+nchannels-1 < BUFFER_SIZE) {
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
    switch(nchannels) {
      case 1:
        buffer.data[buffer.writepos+0] = adc_read_analog(0);
        break;

      case 2:
        buffer.data[buffer.writepos+0] = adc_read_analog(1);
        buffer.data[buffer.writepos+1] = adc_read_analog(0);
        break;

      case 3:
        buffer.data[buffer.writepos+0] = adc_read_analog(1);
        buffer.data[buffer.writepos+1] = adc_read_analog(2);
        buffer.data[buffer.writepos+2] = adc_read_analog(0);
        break;

      case 4:
        buffer.data[buffer.writepos+0] = adc_read_analog(1);
        buffer.data[buffer.writepos+1] = adc_read_analog(2);
        buffer.data[buffer.writepos+2] = adc_read_analog(3);
        buffer.data[buffer.writepos+3] = adc_read_analog(0);
        break;
    }

    buffer.writepos += nchannels;
  }

}

// acknowledge the last command if the command does not immediately
// yield output, e.g. s, C
void _ack(void) {
  printf("OK\n");
}

// signals that the last command encountered an error.
void _err(uint8_t code) {
  printf("ERR%d\n", code);
}

int main(void) {
  uart_init();
  adc_init();
  digital_out_init();
  trigger_init();

  // prime the next channel to be read as channel 0
  adc_read_analog(0);

  printf("READY\n");

  DDRB = _BV(PB7);

  int c, a, r;
  for (;;) {
    // enable interrupts while waiting for serial
    sei();

    c = getchar();

    // interrupts are enabled only for o and f for ease of debugging
    switch(c) {
      case 'o':
      case 'f':
        break;
      default:
        cli();
    }

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
      // a byte is expected after this command that has value 1..4 in ASCII.
      a = getchar();
      nchannels = a - '0';
      if (nchannels > CHANNELS_AVAILABLE || nchannels <= 0) {
        nchannels = CHANNELS_AVAILABLE;
        _err(CHANNEL_OUT_OF_RANGE_ERROR);
      } else {
        // prime channel 0 for read, just in case, even though we should always
        // end with channel 0 primed for next read
        adc_read_analog(0);

        // reset to start of the buffer
        buffer.writepos = 0;

        // reset the trigger
        trigger_reset();

        // this provides feedback of when we have started triggering
        PORTB = 0;

        _ack();
      }
    } else if (c == 'p') {
      //printf("TCNT0=%d OCR0A=%d\n", TCNT0, OCR0A);
      int idx, ch;
      // writepos should never be greater than BUFFER_SIZE, but lets make sure
      if (buffer.writepos > BUFFER_SIZE) buffer.writepos = BUFFER_SIZE;

      for (idx = 0; idx+nchannels-1 < buffer.writepos; idx += nchannels) {
        printf("%d ", idx/nchannels);
        for (ch = 0; ch < nchannels; ++ch) {
          printf("%d ", buffer.data[idx+ch]);
        }
        printf("\n");
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
    } else if (c == 'o' || c == 'f') {
      // this turns on/off a pin, another byte is expected which specified the
      // pin to turn on. Exactly which pin is turned on depends on
      // digital_out.
      a = getchar();
      a = a - '0';
      r = digital_out_write(a, c == 'o');
      if (r == NO_ERROR) _ack();
      else _err(r);
    } else if (c == 't') {
      a = getchar();
      a = a - '0';
      if (a < 1 || a > 9) _err(ARGUMENT_OUT_OF_RANGE_ERROR);
      else {
        trigger_set(a);
        _ack();
      }
    } else _err(UNKNOWN_COMMAND_ERROR);
  }

  return 0;
}
