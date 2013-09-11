arduino-ADS7825
===============

Simple arduino program to interface with an ADS7825, a 4 channel 16bit ADC from TI.

Communication
=============

The program supports 3 commands:

- `r`:  reads the 4 channels, printing the results as ASCII to serial immediately.
- `s:`: starts a continuous scan of all 4 channels whenever an external trigger
        is received, possibly after some frequency division. Data is stored
        sequentially into an internal buffer. Scan stops when buffer is full.
- `p`:  prints the contents of the buffer in ASCII, up to the last write
        position.
- `b`:  prints the contents of the buffer in binary. The byte stream will
        contain 16 bit signed integers, send LSB first. The first 16 bit
        integer is the number of 16 bit integers to follow.

Notes
=====

This program is written as a quick hack to replace a broken 16bit ADC in an existing instrument, and thus *is a little crap*.
