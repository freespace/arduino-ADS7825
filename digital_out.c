#include <avr/io.h>

#include "errors.h"
#include "digital_out.h"

void digital_out_init(void) {
  DDRL = 0xFF; 
  PORTL = 0;
}

void digital_out_port_write(uint8_t pins) {
  PORTL = pins;
}

uint8_t digital_out_write(uint8_t pin, uint8_t state) {
  if (pin < 0 || pin > DIGITAL_OUT_PINS_AVAILABLE-1) return DIGITAL_PIN_OUT_OF_RANGE_ERROR;

  if (state) PORTL |= _BV(pin);
  else PORTL &= ~_BV(pin);

  return NO_ERROR;
}
