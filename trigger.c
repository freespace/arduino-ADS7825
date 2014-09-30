#include <avr/io.h>
#include <avr/interrupt.h>

void trigger_reset(void) {
  TCNT0 = 0;
}

void trigger_set(uint8_t n) {
  // set the value at which we roll over, using OCR0A. We enforce the
  // condition that n is >=1 because triggering for every 0 external trigger
  // seen makes no sense.
  if (n < 1) n = 1;

  // we need the -1 because in CTC mode, the counter is cleared
  // after reaching TOP on the NEXT pulse, which is also when
  // the interrupt is issued. This has been confirmed experimentally,
  // where with OCR0A set to 1:
  // TCNT = 0, 1, *0, 1, *0, ...
  //
  // Where * indicates interrupt.
  n = n - 1;

  // it is not sufficient to just update OCR0A due to double buffering feature
  // of counter 0. In some waveform generation modes (WGM), OCR0A is not
  // updated immediately, resulting in early or late triggers. This is
  // resolved by setting TCCR0A and TCCR0B to 0, which forces WGM mode 0 in
  // which OCR0A update occurs immediately. After OCR0A is updated TCCR0A and
  // TCCR0B are restored to their original values.
  uint8_t a = TCCR0A;
  uint8_t b = TCCR0B;

  TCCR0A = 0x00;
  TCCR0B = 0x00;

  OCR0A = n;

  TCCR0A = a;
  TCCR0B = b;
}

void trigger_init(void) {
  // setup counter0 issue an interrupt for every n external trigger seen.

  // OC0A and OC0B disconnected
  // CTC waveform generation mode, clearing the timer on match
  TCCR0A = 0x00 | _BV(WGM01) | _BV(WGM00);

  // Set T0 as external clock, clock on falling edge, set fast PWM
  TCCR0B = 0x00 | _BV(CS02) | _BV(CS01) | _BV(WGM02);

  // issue an interrupt TCNT0 overflows OCR0A
  TIMSK0 = 0x00 | _BV(TOIE0);

  // by default trigger on every trigger
  trigger_set(1);
}
