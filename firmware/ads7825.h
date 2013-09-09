#ifndef ADS7825_H
#define ADS7825_H

#include <avr/io.h>

// control pins are in port A
#define CONTROL_PORT      (PORTA)
#define CONTROL_PORT_DDR  (DDRA)
#define CONTROL_PORT_PIN  (PINA)

#define A0                (PA0)
#define A1                (PA1)
#define BYTE              (PA2)
#define RC                (PA3)
#define BUSY              (PA4)

// data pins are in port C
#define DATA_PORT_PIN     (PINC)
#define DATA_PORT_DDR          (DDRC)

int adc_init(void);
int16_t adc_read_analog(uint8_t next_chan);

#endif
