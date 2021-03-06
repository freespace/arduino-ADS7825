# arduino-ADS7825

Simple Arduino program to interface with an ADS7825, a 4 channel 16bit ADC from TI. Note that the C version is way ahead of the Arduino implementation, but the Arduino implementation should still work.

# Communication

When firmware is ready to accept commands, it will output `READY\n`.

The program supports the following commands:

- `r`:  reads the 4 channels, printing the results as ASCII to serial immediately.

- `sN:`:starts a continuous scan of N channels whenever an external trigger
        is received, possibly after some frequency division. Data is stored
        sequentially into an internal buffer. Scan stops when buffer is full.

- `p`:  prints the contents of the buffer in ASCII, up to the last write
        position.

- `b`:  prints the contents of the buffer in binary. The byte stream will
        contain 16 bit signed integers, sent LSB first. The first 16 bit
        integer is the number of 16 bit integers to follow.

- `BX`: prints the first X 16 bit unsigned integers, sent LSB first.
        X is expected be 2 characters encoding 64 values each. Each
        character is produced by adding the desired value (0..63) onto
        the ASCII value for '0'. The least significant byte is expected
        first. Note that like `b`, the first 16 bit integers is the
        number of 16 bit integers to follow.

- `c`:  sends a 16 bit integer, LSB first, of how many integers are currently
        in the buffer.
- `oN`:
- `fN`: turns on/off pin N. Which physical pin this corresponds to depends
        entirely on `digital_out`. Currently it maps to digital pins 49..47
        for values 0..7, which is port L on the ATMega1280.

- `tN`: sets the trigger divider. For example, if N=1, then every trigger will
        result in a scan. If N=2, every OTHER trigger will result in a scan.

- `xN`: sets the exposure value, which is the number of readings per trigger.
        N is between 1-9 only, and defaults to 1.

Commands that do not result in immediate output, such as `s` will output either
`OK` or `ERRx` to indicate success or error, followed by `\n`. `x` is an error
code that will specify the error that occured. See `errors.h` for list of error codes.

Where not specified, numbers are always sent as ASCII to facilitate ease
of manual operation.

## Interrupts

For all commands other than `o` and `f`, interrupts are disabled when the first
character, and remains disabled until the commands has been processed.

Having interrupts enabled during `o` and `f` allows self testing of trigger and scan
functionality.

# Schematic

Two version of this board exists. One as an Arduino Mega shield, the other as a standalone board. The arduino shield schematic is shown below.

![Arduino Mega shield schematic](https://raw.github.com/freespace/arduino-ADS7825/master/EDA/arduino_mega_adc_shield.gif)

# Notes

This program is written as a quick hack to replace a broken 16bit ADC in an
existing instrument, and thus *is a little crap*. The EDA for this project is
based on the breadboard version that I had constructed, and as of 3rd of Feb
2014 is untested.

### Update
2014.09.15: The EDA board has been constructed and has been found to work.
