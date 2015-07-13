#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <limits.h>

#include <util/delay_basic.h>

#include "errors.h"
#include "ads7825.h"

#define SET(port,bit)  (port |= _BV(bit))
#define CLEAR(port,bit) (port &=~(_BV(bit)))

uint8_t _exposures = 1;

inline int16_t _read_once(void) {
  // begin a conversion by pull RC low
  CLEAR(CONTROL_PORT, RC);
  
  // t1, Convert Pulse Width, must be no less than 0.04 us and no more than
  // 12 us. In other words, no less than 40ns, and since one cycle at 16 MHz
  // is 62.5 ns, we can just delay for a single cycle
  _NOP();

  // now make RC high
  SET(CONTROL_PORT, RC);

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
  int16_t code = DATA_PORT_PIN;
  code <<= 8;

  // now for the low byte
  SET(CONTROL_PORT, BYTE);
  _NOP(); _NOP();
  code |= DATA_PORT_PIN;

  return code;
}

int adc_init(void) {
  // all of control port is output but the busy bit
  CONTROL_PORT_DDR = 0xFF;
  CLEAR(CONTROL_PORT_DDR, BUSY);

  DATA_PORT_DDR = 0x00;

  return 0;
}

/**
 * Sets the number of readings performed by adc_read_analog when it is
 * called.
 */ 
uint8_t adc_set_exposures(uint8_t exp) {
  if (exp == 0) return ARGUMENT_OUT_OF_RANGE_ERROR;
  else {
    _exposures = exp;
    return NO_ERROR;
  }
}

/**
 * This function is NOT wrong. It is written this way using unsigned integers
 * because I only care about unsigned integer overflows.
 *
 * While ADS7825 returns signed integers, I only measure positive integers and
 * accumulate them according to the number of exposures. This naturally leads
 * to overflow into negative values at some point.
 *
 * This overflow is fine because all I need to do is interpret the number as
 * unsigned and I recover the actual sum-of-positive values. The difficult is
 * knowing when I have overflowed back to positive values, because then it
 * appears the same signed or unsigned.
 * 
 * This method is built on the assumption I only add positive values, and I
 * only interpret the result as unsigned integers.
 */
inline uint16_t addint(uint16_t a, uint16_t b) {
  uint16_t c = a+b;
  if (c < a || c < b) return 0xFFFF;
  else return c;
}

/**
  Reads in n number of conversion values, and sets the next channel to be
  sampled. n is determined by adc_set_exposures and defaults to 1.

  It is recommended that, if this code is called outside of an ISR, for it to
  be surrounded by cli() and sei() to avoid interrupts occuring during
  execution of the function.

  Returned value is 2s complemented 16 bit integer.

  Note that when exposures is set the result should be interpreted as uint16_t
  */
int16_t adc_read_analog(uint8_t next_chan) {
  // do exposure-1 readings without touching A0 and A1
  int16_t code = 0;
  uint8_t cnt = 0;
  for (; cnt < _exposures-1; ++cnt) {
    code = addint(code, _read_once());
  }

  // for the last read we need to setup for next_chan, so we modify
  // A0 and A1 as needed
  next_chan&0x1?SET(CONTROL_PORT, A0):CLEAR(CONTROL_PORT, A0);
  next_chan&0x2?SET(CONTROL_PORT, A1):CLEAR(CONTROL_PORT, A1);

  // and doe on more read
  code = addint(code, _read_once());

  return code;
}

