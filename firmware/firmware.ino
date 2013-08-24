/**
 * This firmware is intended to interface with a ADS7825 from TI/BB, and
 * provide digital codes to the control software over USB-Serial.
 */

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
  Serial.println(millis());
}

/**
  Waits for BUSY to go high
  */
void wait_busy() {
  while(digitalRead(_BUSY) == LOW) {
    digitalWrite(_LED, ~digitalRead(_BUSY));
  }
}

/**
  Reads in the current conversion value, and sets the next channel to be
  sampled.

  Returned value is 2s complemented 16 bit integer.
  */
int16_t read_analog(uint8_t next_chan) {

  // select the channel for the next read
  digitalWrite(_A0, next_chan&0x1?HIGH:LOW);
  digitalWrite(_A1, next_chan&0x2?HIGH:LOW);

  // we really don't need any interrupts messing the timings in here up
  noInterrupts();
  
  // begin a conversion
  digitalWrite(_RC, LOW);
  delayMicroseconds(1);
  digitalWrite(_RC, HIGH);

  interrupts();

  // wait for busy to go high
  wait_busy();

  // pre-select the high byte for reading
  digitalWrite(_BYTE, LOW);
  delayMicroseconds(1);

  // use port manipulation to read the data pins. Note that our pinmapping
  // is such that port C maps exactly onto the parallel output from the
  // ADS7825.
  uint8_t hbyte = PINC;

  // now for the low byte
  digitalWrite(_BYTE, HIGH);
  delayMicroseconds(1);

  uint8_t lbyte = PINC;


  return (hbyte<<8 | lbyte);
}

volatile uint8_t trigcnt = 0;
void trig_ISR() {
  trigcnt += 1;
}


void loop() {
  loop2();
}

int16_t cnt = 0;
int16_t buf[_BUF_SIZE];
uint16_t writepos = 0;

void loop2() {
  // XXX this should be 10, but is 5 for debugging purposes
  if (trigcnt>=10) {
    trigcnt = 0;
    if (writepos < _BUF_SIZE) {
      buf[writepos+0] = read_analog(1);
      buf[writepos+1] = read_analog(2);
      buf[writepos+2] = read_analog(3);
      buf[writepos+3] = read_analog(0);

      writepos += 4;
    } else {
      if (writepos != 0xFFFF) {
        Serial.println(millis());
        writepos = 0xFFFF;

        for (int idx = 0; idx < _BUF_SIZE; idx+=4) {
          Serial.print(idx/4);
          Serial.print(" ");
          Serial.print(buf[idx+0]);
          Serial.print(" ");
          Serial.print(buf[idx+1]);
          Serial.print(" ");
          Serial.print(buf[idx+2]);
          Serial.print(" ");
          Serial.print(buf[idx+3]);
          Serial.println();
        }
      }
    }
  }
}

void loop1() {
  buf[0] = read_analog(1);
  buf[1] = read_analog(2);
  buf[2] = read_analog(3);
  buf[3] = read_analog(0);
  delay(10);
  Serial.print(cnt);
  Serial.print(" ");
  Serial.print(buf[0]);
  Serial.print(" ");
  Serial.print(buf[1]);
  Serial.print(" ");
  Serial.print(buf[2]);
  Serial.print(" ");
  Serial.print(buf[3]);
  Serial.println();
  cnt += 1;
 
}

uint8_t trigstate = LOW;
void loop0() {
  // XXX trigger on a rising edge
  if (trigstate != digitalRead(_TRIG_PIN) && trigstate == HIGH) {
    trigcnt += 1;
    trigstate = !trigstate;
  }

  if (trigcnt >= 10) {
    trigcnt = 0;
    buf[0] = read_analog(1);
    buf[1] = read_analog(2);
    buf[2] = read_analog(3);
    buf[3] = read_analog(0);

    Serial.print(cnt);
    Serial.print(" ");
    Serial.print(buf[0]);
    Serial.print(" ");
    Serial.print(buf[1]);
    Serial.print(" ");
    Serial.print(buf[2]);
    Serial.print(" ");
    Serial.print(buf[3]);
    Serial.println();
    cnt += 1;
  }
}
