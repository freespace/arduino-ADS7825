#ifndef DIGITAL_OUT_H
#define DIGITAL_OUT_H

#define DIGITAL_OUT_PINS_AVAILABLE  (8)

enum {
  D49,
  D48,
  D47,
  D46,
  D45,
  D44,
  D43,
  D42
};

/**
 * Sets up Port L as output, which on the Arduino Mega
 * maps to pins 42-47.
 */ 
void digital_out_init(void);

/**
 * Low level function to bit-bang port L.
 *
 * Mapping:
 *
 *   0 = D49
 *   1 = D48
 *   2 = D47
 *   3 = D46
 *   4 = D45
 *   5 = D44
 *   6 = D43
 *   7 = D42
 *
 * For ease of use, enums exists for D42..D49.
 */
void digital_out_bulk_write(uint8_t pins);

/**
 * Turns on/off the specified pin. Same mapping as digital_out_bulk_write.
 *
 * Returns 0 if success, error code otherwise.
 */ 
uint8_t digital_out_write(uint8_t pin, uint8_t state);

#endif
