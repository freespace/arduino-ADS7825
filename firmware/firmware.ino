/**
 ****************************************************************************
 *THIS CODE IS OBSOLETE. SEE main.c IN PROJECT ROOT FOR THE CURRENT VERSION *
 ****************************************************************************
 * This firmware is intended to interface with a ADS7825 from TI/BB, and
 * provide digital codes to the control software over USB-Serial.
 * 
 * The software responds to the following serial commands:
 *
 * - r:  read all 4 channels, return results immediately via serial
 * - s:  on receiving this command, the arduino will immediately begin to
 *       capture data into its buffer when triggered, stopping when the
 *       buffer is filled.
 *
 * - p:  prints the current valid contents of the buffer. One line is emitted
 *       for each scan of 4 channels, each line containing 4 codes separated
 *       by space. The last line of buffer content is followed by a line
 *       containing 'END'. All lines are terminated by '\n'.
 */

#ifndef __builtin_avr_delay_cycles
inline void __builtin_avr_delay_cycles(unsigned long __n) {
  while(__n) __n--;
}
#endif

// Adding _ prefix b/c arduino api uses some of these names otherwise
#define     _A0       (22)  // PB3
#define     _A1       (23)  // PB4
#define     _BYTE     (24)
#define     _RC       (25)
#define     _BUSY     (26)

// these pins map to PORTC
#define     _D0       (37)
#define     _D1       (36)
#define     _D2       (35)
#define     _D3       (34)
#define     _D4       (33)
#define     _D5       (32)
#define     _D6       (31)
#define     _D7       (30)

#define     _LED      (13)

#define     _TRIG_PIN (18)
#define     _TRIG_INT (5)

// the trigger frequency is divided by _DIVIDER. e.g. if the trigger frequency
// is 100Hz, and _DIVIDER is 10, then sampling occurs at 10Hz
#define     _DIVIDER  (10)

// *4 b/c we are sampling 4 channels. We assume each element of buf is enough
// to hold a 16bit value
#define     _BUF_SIZE (512*4)

void setup() {
  uint8_t outputs[] = {_A0, _A1, _BYTE, _RC, _LED};
  uint8_t inputs[] = {_BUSY, _D0, _D1, _D2, _D3, _D4, _D5, _D6, _D7};

  for (int idx = 0; idx < sizeof outputs; ++idx) {
    pinMode(outputs[idx], OUTPUT);
  }

  for (int idx = 0; idx < sizeof inputs; ++idx) {
    pinMode(inputs[idx], INPUT);
  }

  // put the chip into read mode by default
  digitalWrite(_RC, HIGH);

  wait_busy();

  // attach an interrupt to our trigger
  attachInterrupt(_TRIG_INT, trig_ISR, FALLING);

  Serial.begin(115200);
  Serial.println("Ready");
}

/**
  Waits for BUSY to go high
  */
inline void wait_busy() {
  digitalWrite(_LED, LOW);
  while(digitalRead(_BUSY) == LOW);
  digitalWrite(_LED, HIGH);
}

/**
  Reads in the current conversion value, and sets the next channel to be
  sampled.

  Returned value is 2s complemented 16 bit integer.
  */
inline int16_t read_analog(uint8_t next_chan) {

  // select the channel for the next read
  digitalWrite(_A0, next_chan&0x1?HIGH:LOW);
  digitalWrite(_A1, next_chan&0x2?HIGH:LOW);

  // begin a conversion
  digitalWrite(_RC, LOW);

  // t1, Convert Pulse Width, must be no less than 0.04 us and no more than
  // 12 us. In other words, no less than 40ns, and since one cycle at 16 MHz
  // is 62.5 ns, we can just delay for a single cycle
  __builtin_avr_delay_cycles(1);

  digitalWrite(_RC, HIGH);

  // t4, BUSY LOE, is at most 21 us
  delayMicroseconds(22);

  // select the high byte for reading
  digitalWrite(_BYTE, LOW);
  
  // Since we are running at 16MHz, each cycle is 62.5 ns. t12, bus access and
  // BYTE delay, is no more than 83 ns from datasheet, so 2 nops will cover it.
  __builtin_avr_delay_cycles(2);

  // use port manipulation to read the data pins. Note that our pinmapping
  // is such that port C maps exactly onto the parallel output from the
  // ADS7825.
  uint16_t code = PINC;
  code <<=8;

  // now for the low byte
  digitalWrite(_BYTE, HIGH);
  __builtin_avr_delay_cycles(2);
  code |= PINC;

  return code;
}

// buffer for 16bit values from the ADC
volatile int16_t buf[_BUF_SIZE];
volatile uint16_t writepos = 0;
volatile uint8_t trigcnt = 0;
void trig_ISR() {
  if (++trigcnt == _DIVIDER) {
    trigcnt = 0;
    if (writepos < _BUF_SIZE) {
      buf[writepos++] = read_analog(1);
      buf[writepos++] = read_analog(2);
      buf[writepos++] = read_analog(3);
      buf[writepos++] = read_analog(0);
    }
  }
}

void loop() {
  int c = Serial.read();
  if (c == 'r') {
    // prime the ADS7825 to scan channel 0
    read_analog(0);
    Serial.print(read_analog(1));Serial.print(" ");
    Serial.print(read_analog(2));Serial.print(" ");
    Serial.print(read_analog(3));Serial.print(" ");
    Serial.print(read_analog(0));Serial.println();
  } else if (c == 's') {
    noInterrupts();
    // prime the first channel
    read_analog(0);
    trigcnt = 0;
    writepos = 0;
    interrupts();
  } else if (c == 'p') {
    for (int idx = 0; idx < writepos; idx += 4) {
      Serial.print(idx/4);Serial.print(" ");
      Serial.print(buf[idx+0]);Serial.print(" ");
      Serial.print(buf[idx+1]);Serial.print(" ");
      Serial.print(buf[idx+2]);Serial.print(" ");
      Serial.print(buf[idx+3]);Serial.println();
    }
    Serial.println("END");
  } else if (c != -1) Serial.println("???");
}
