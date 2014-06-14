#!/usr/bin/env python
"""
Simple interface to an arduino interface to the ADS7825. For more
details see:

  https://github.com/freespace/arduino-ADS7825

Note that in my testing, the ADS7825 is very accurate up to 10V, to the
point where there is little point, in my opinion, to write calibration
code. The codes produce voltages that are within 0.01V of the value set
using a HP lab PSU.
"""

import serial
import time

def c2v(c):
  return float(c)/(0x7FFF)*10

class ADS7825(object):
  def __init__(self, port='/dev/ttyUSB0'):
    super(ADS7825, self).__init__()
    self._ser = serial.Serial(port, baudrate=57600)

    # do a dummy read to flush the serial pipline. This is required because
    # the arduino reboots up on being connected, and emits a 'Ready' string
    self._write('r')
    self._readline()

  def _write(self, x):
    # print 'sent',x
    self._ser.write(x)
    self._ser.flush()

  def _readline(self):
    return self._ser.readline()

  def read(self, raw=False):
    """
    Returns a 4-tuple of floats, containing voltage measurements, in volts
    from channels 0-3 respectively. Voltage range is +/-10V
    """
    self._write('r')
    codes = map(float,map(str.strip,self._readline().split(',')))
    if raw:
      return codes
    else:
      volts = map(c2v, codes)
      return volts

  def scan(self):
    """
    Asks the firmware to beginning scanning the 4 channels on external trigge.
    """
    self._write('s')

  def read_scan(self, nscans, binary=True):
    """
    Gets from the firmware the scans it buffered from scan(). You must
    supply the number of scans to retrieve.

    If binary is set to True, then 'b' is used to retrieve the buffer instead
    of 'p'. Defaults to True.

    Returns a list of voltage measurements. , e.g.

      [v0_0, v1_0, v2_0, v3_0, v0_1, v1_1, v2_1, v3_1 ...
        v0_n-1, v1_n-1, v2_n-1, v3_-1]
    """
    if binary:
      self._ser.flushInput()
      self._write('b')
      def read16i():
        b = self._ser.read(2)
        import struct
        return struct.unpack('<h', b)[0]

      nints = read16i()

      if nints < nscans*4:
          raise Exception("Premature end of buffer. ADC probably couldn't keep up. Codes available: %d"%(nints))

      codes = list()
      while nints:
        codes.append(read16i())
        nints -= 1

      return map(c2v, codes)[:nscans*4]
    else:
      self._write('p')

      volts = list()
      done = False
      while not done:
        line = self._readline().strip()
        if line == 'END':
          raise Exception("Premature end of buffer. ADC probably couldn't keep up. Lines read: %d"%(len(volts)//4))

        ints = map(int,line.split(' '))
        index = ints[0]
        codes = ints[1:]
        assert(len(codes) == 4)
        volts += map(c2v, codes)

        if index == nscans-1:
          done = True

      # read to the end of the buffer
      line = ''
      while line != 'END':
        line = self._readline().strip()

      return volts

  def __del__(self):
    del self._ser

def test_read(port='/dev/ttyUSB0', raw=False):
  daq = ADS7825(port=port)
  volts = daq.read(raw=raw)
  assert(len(volts) == 4)
  return volts

def test_scan():
  nscans = 500
  daq = ADS7825()
  daq.scan(nscans)
  volts = daq.read_scan(nscans)

  assert(len(volts) == 4*nscans)

if __name__ == '__main__':
  import sys
  print test_read(sys.argv[1], True)
  print test_read(sys.argv[1])
