arduino-ADS7825
===============

Simple arduino program to interface with an ADS7825, a 4 channel 16bit ADC from TI.

Communication
=============

The program supports 3 commands:

- `r`: reads the 4 channels, printing the results as ASCII to serial immediately.
- `sN.:`: performs `N` scans of the 4 channels, storing the results in a buffer.
          Note that the full stop (.) is required
- `p`: prints the buffered data from a previous `sN` command.

Notes
=====

This program is written as a quick hack to replace a broken 16bit ADC in an existing instrument, and thus *is a little crap*.
