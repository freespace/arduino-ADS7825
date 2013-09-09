#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

#include <util/delay_basic.h>

#include "ads7825.h"

#define SET(port,bit)  (port |= _BV(bit))
#define CLEAR(port,bit) (port &=~(_BV(bit)))

int adc_init(void) {
  // all of control port is output but the busy bit
  CONTROL_PORT_DDR = 0xFF;
  CLEAR(CONTROL_PORT_DDR, BUSY);

  DATA_PORT_DDR = 0x00;

  return 0;
}

/**
  Reads in the current conversion value, and sets the next channel to be
  sampled.

  Returned value is 2s complemented 16 bit integer.
  */
int16_t adc_read_analog(uint8_t next_chan) {
  // select the channel for the next read
  CONTROL_PORT = (CONTROL_PORT&0xFC) | (next_chan&0xF3);

  // we really don't need any interrupts messing the timings in here up
  cli();
  
  // begin a conversion by pull RC low
  CLEAR(CONTROL_PORT, RC);
  
  // t1, Convert Pulse Width, must be no less than 0.04 us and no more than
  // 12 us. In other words, no less than 40ns, and since one cycle at 16 MHz
  // is 62.5 ns, we can just delay for a single cycle
  _NOP();

  // now make RC high
  SET(CONTROL_PORT, RC);

  sei();

  // wait for busy to go high
  loop_until_bit_is_set(CONTROL_PORT_PIN, BUSY);

  // elect the high byte for reading
  CLEAR(CONTROL_PORT, BYTE);

  // Since we are running at 16MHz, each cycle is 62.5 ns. t12, bus access and
  // BYTE delay, is no more than 83 ns from datasheet, so 2 nops will cover it.
  _NOP(); _NOP();

  // use port manipulation to read the data pins. Note that our pinmapping
  // is such that port C maps exactly onto the parallel output from the
  // ADS7825.
  uint16_t code = DATA_PORT_PIN;
  code <<= 8;

  // now for the low byte
  SET(CONTROL_PORT, BYTE);
  _NOP(); _NOP();
  code |= DATA_PORT_PIN;

  return code;
}
