arduino-ADS7825
===============

Simple arduino program to interface with an ADS7825, a 4 channel 16bit ADC from TI.

Communication
=============

When firmware is ready to accept commands, it will output `READY\n`.

The program supports the following commands:

- `r`:  reads the 4 channels, printing the results as ASCII to serial immediately.
- `sN:`: starts a continuous scan of N channels whenever an external trigger
        is received, possibly after some frequency division. Data is stored
        sequentially into an internal buffer. Scan stops when buffer is full.
- `p`:  prints the contents of the buffer in ASCII, up to the last write
        position.
- `b`:  prints the contents of the buffer in binary. The byte stream will
        contain 16 bit signed integers, send LSB first. The first 16 bit
        integer is the number of 16 bit integers to follow.
- `c`:  sends a 16 bit integer, LSB first, of how many integers are currently
        in the buffer.

Commands that do not result in immediate output, such as `s` will output either
`OK` or `ERRx` to indicate success or error, followed by `\n`. `x` is an error
code that will specify the error that occured.

Schematic
=========

Two version of this board exists. One as an Arduino Mega shield, the other as a standalone board. The arduino shield schematic is shown below.

![Arduino Mega shield schematic](https://raw.github.com/freespace/arduino-ADS7825/master/EDA/arduino_mega_adc_shield.gif)

Notes
=====

This program is written as a quick hack to replace a broken 16bit ADC in an
existing instrument, and thus *is a little crap*. The EDA for this project is
based on the breadboard version that I had constructed, and as of 3rd of Feb
2014 is untested.

## Update
2014.09.15: The EDA board has been constructed and has been found to work.

