/**
 * This firmware is intended to interface with a ADS7825 from TI/BB, and
 * provide digital codes to the control software over USB-Serial.
 */

// Adding _ prefix b/c arduino api uses some of these names otherwise
#define     _A0       (22)
#define     _A1       (23)
#define     _BYTE     (24)
#define     _RC       (25)
#define     _BUSY     (26)

#define     _D0       (37)
#define     _D1       (36)
#define     _D2       (35)
#define     _D3       (34)
#define     _D4       (33)
#define     _D5       (32)
#define     _D6       (31)
#define     _D7       (30)

#define     _LED       (13)

#define     _TRIGGER   (40)

void setup() {
  uint8_t outputs[] = {_A0, _A1, _BYTE, _RC, _LED};
  uint8_t inputs[] = {_BUSY, _D0, _D1, _D2, _D3, _D4, _D5, _D6, _D7, _TRIGGER};

  for (int idx = 0; idx < sizeof outputs; ++idx) {
    pinMode(outputs[idx], OUTPUT);
  }

  for (int idx = 0; idx < sizeof inputs; ++idx) {
    pinMode(inputs[idx], INPUT);
  }

  // put the chip into read mode by default
  digitalWrite(_RC, HIGH);

  wait_busy();

  // set the next channel to be read to 0
  read_analog(0);

  Serial.begin(19200);
  Serial.println("Ready");
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
  */
int16_t read_analog(uint8_t next_chan) {
  // select the channel for the next read
  digitalWrite(_A0, next_chan&0x01);
  digitalWrite(_A1, next_chan>>1);

  // pre-select the high byte for reading
  digitalWrite(_BYTE, LOW);

  // begin a conversion
  digitalWrite(_RC, LOW);
  delayMicroseconds(1);
  digitalWrite(_RC, HIGH);

  // wait for busy to go high
  wait_busy();

  // use port manipulation to read the data pins. Note that our pinmapping
  // is such that port C maps exactly onto the parallel output from the
  // ADS7825.
  uint8_t hbyte = PINC;

  // now for the high byte
  digitalWrite(_BYTE, HIGH);
  delayMicroseconds(1);

  uint8_t lbyte = PINC;

  return (hbyte<<8 | lbyte)&0xFFFF;
}

void ser_print_si(char * s, int i) {
  Serial.print(s);
  Serial.print(i, DEC);
}

void ser_print_sx(char * s, int i) {
  Serial.print(s);
  Serial.print(i, HEX);
}

int16_t cnt = 0;
void loop() {
  Serial.print(cnt, DEC);
  ser_print_si(" ", read_analog(1));
  ser_print_si(" ", read_analog(2));
  ser_print_si(" ", read_analog(3));
  ser_print_si(" ", read_analog(0));
  Serial.println();
  cnt += 1;
}
