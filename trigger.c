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

  OCR0A = n;

  // in fast PWM mode, OCR0A is only updated when TCNT0 reaches BOTTOM. If we
  // don't force this to happen, then the old value in OCR0A will be active
  // for one more interrupt. Therefore we force TCNT0 to overflow by setting
  // its clock to the system clock...
  TCCR0B = 0x00 | _BV(CS00) | _BV(WGM02);

  // then waiting until OCR0A has the value we want
  while (OCR0A != n);

  // restore external clock source
  TCCR0B = 0x00 | _BV(CS02) | _BV(CS01) | _BV(WGM02);
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
