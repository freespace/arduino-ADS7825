#!/usr/bin/env python
"""
Simple interface to an arduino interface to the ADS7825. For more
details see:

  https://github.com/freespace/arduino-ADS7825

Note that in my testing, the ADS7825 is very accurate up to 10V, to the
point where there is little point, in my opinion, to write calibration
code. The codes produce voltages that are within 0.01V of the value set
using a HP lab PSU.

TODO:
  - Implement self test using don/doff commands. Use read function to
    determine if don/doff is working
  - Use either PWM driver or another chip to generate a clock signal
    and determine the maximum sampling rate of the system
    - It will be limited by how fast we can service interrupts, which
      in turn will be limited by how fast the firmware is capable of
      reading from the ADC
"""

from __future__ import division
import serial
import time

def c2v(c, signed=True):
  """
  If signed is True, returns a voltage between -10..10 by interpreting c
  as a signed 16 bit integer. This is the default behaviour.

  If signed is False, returns a voltage between 0..20 by interpreting c
  as an UNSIGNED 16 bit integer.
  """
  if signed:
    return 10.0 * float(c)/(0x7FFF)
  else:
    return 20.0 * float(int(c)&0xFFFF)/(0xFFFF)

def find_arduino_port():
  from serial.tools import list_ports
  comports = list_ports.comports()
  for path, name, vidpid in comports:
    if 'FT232' in name:
      return path

  if len(comports):
    return comports[0][0]
  else:
    return None


class ADS7825(object):
  # this indicates whether we interpret the integers from the arduino
  # as signed or unsigned values. If this is True, then voltages will be
  # returned in the range -10..10. Otherwise voltages will be returned
  # in the range 0..10.
  #
  # This is useful when doing multiple exposures with unipolar signals which
  # would otherwise overflow into negative values when signed.
  signed=True

  def __init__(self, port=find_arduino_port(), verbose=False):
    super(ADS7825, self).__init__()
    self._verbose = verbose
    self._ser = serial.Serial(port, baudrate=250000)

    # Let the bootloader run
    time.sleep(2)
    # do a dummy read to flush the serial pipline. This is required because
    # the arduino reboots up on being connected, and emits a 'Ready' string
    while self._ser.inWaiting():
      c = self._ser.read()
      if verbose:
        print 'ads7825>>',c

  def _c2v(self, c):
    return c2v(c, signed=self.signed)

  def _write(self, x, expectOK=False):
    self._ser.write(x)
    self._ser.flush()

    if self._verbose:
      print '>',x

    if expectOK:
      l = self._readline()
      assert l == 'OK', 'Expected OK got: '+l

  def _readline(self):
    return self._ser.readline().strip()

  def _read16i(self, count=1):
    b = self._ser.read(2*count)
    import struct
    fmtstr = '<'+ 'h' * count
    ints = struct.unpack(fmtstr, b)
    if count == 1:
      return ints[0]
    else:
      return ints

  def set_exposures(self, exposures):
    """
    Sets the exposure value, which is the number of readings per read request.
    This affects read and scan, and defaults to 1.
    """
    exposures = int(exposures)
    aschar = chr(ord('0')+exposures)
    self._write('x'+aschar, expectOK=True)

  def read(self, raw=False):
    """
    Returns a 4-tuple of floats, containing voltage measurements, in volts
    from channels 0-3 respectively. Voltage range is +/-10V
    """
    self._write('r')
    line = self._readline()
    codes = map(float,map(str.strip,line.split(',')))

    if raw:
      return codes
    else:
      volts = map(self._c2v, codes)
      return volts

  def scan(self, nchannels=4):
    """
    Asks the firmware to beginning scanning the 4 channels on external trigger.

    Note that scans do NOT block serial communication, so read_scans will return
    the number of scans currently available.
    """
    self._write('s'+str(nchannels), expectOK=True)

  @property
  def buffer_writepos(self):
    """
    Returns the write position in the buffer, which is an indirect measure of the number
    of scans, with the write position incremeneting by nchannels for every scan triggered.
    """
    self._write('c')
    return self._read16i()


  def output_on(self, output):
    """
    Turns the specified output
    """
    assert output >= 0 and output < 10, 'Output out of range'
    self._write('o'+str(output), expectOK=True)


  def output_off(self, output):
    """
    Turns off the specified output
    """
    assert output >= 0 and output < 10, 'Output out of range'
    self._write('f'+str(output), expectOK=True)

  def set_trigger_divider(self, n):
    """
    Divides the incoming trigger signal by n, triggering every nth trigger.

    Valid range is 1..9 currently.
    """
    assert n > 0 and n < 10
    self._write('t'+str(n), expectOK=True)

  def read_scans(self, nscans, binary=True, nchannels=4):
    """
    Gets from the firmware the scans it buffered from scan(). You must
    supply the number of scans (nscans) to retrieve. Each scan consists
    of nchannels number of 16 bit ints.

    nchannels: the number of channels scanned per scan

    If binary is set to True, then 'b' is used to retrieve the buffer instead
    of 'p'. Defaults to True.

    Returns a list of voltage measurements. , e.g.

      [v0_0, v1_0, v2_0, v3_0, v0_1, v1_1, v2_1, v3_1 ...
        v0_n-1, v1_n-1, v2_n-1, v3_-1]


    If called before nscans have been required, you will get an exception.
    Just wait a bit longer, and check also that nscans does not exceed the
    buffer allocated in the firmware. e.g. if buffer can only hold 10 scans,
    and you want 20, then this method will ALWAYS throw an exception.

    You should NOT call this during data acquisition because if the processor
    is forced to handle communication then it will miss triggers because to
    preserve data integrity interrupts are disabled when processing commands.
    """
    nscans = int(nscans)
    nchannels = int(nchannels)

    self._ser.flushInput()

    if binary:
      nints = self.buffer_writepos

      if nints < nscans*nchannels:
        raise Exception("Premature end of buffer. ADC probably couldn't keep up. Codes available: %d need %d"%(nints, nscans*nchannels))

      # ask for binary print out of the buffer, and the buffer might have
      # updated we get the up to date count of ints to expect
      self._write('b')
      nints = self._read16i()

      codes = self._read16i(count=nints)

      return map(self._c2v, codes)[:nscans*nchannels]
    else:
      self._write('p')

      volts = list()
      done = False
      while not done:
        line = self._readline()
        if line == 'END':
          raise Exception("Premature end of buffer. ADC probably couldn't keep up. Lines read: %d"%(len(volts)//4))

        ints = map(int,line.split(' '))
        index = ints[0]
        codes = ints[1:]
        assert(len(codes) == 4)
        volts += map(self._c2v, codes)

        if index == nscans-1:
          done = True

      # read to the end of the buffer
      while self._readline() != 'END':
        pass

      return volts

  def close(self):
    self._ser.close()
    self._ser = None

class Test():
  @classmethod
  def setup_all(self):
    ardport = find_arduino_port()
    import readline
    userport = raw_input('Serial port [%s]: '%(ardport))
    if len(userport) == 0:
      userport = ardport

    self.adc = ADS7825(ardport, verbose=False)

  def _banner(self, text):
    l = len(text)
    print
    print text
    print '-'*l

  def test_read(self):
    volts = self.adc.read()
    print 'Volts: ', volts
    assert len(volts) == 4

  def test_exposures(self):
    self.adc.set_exposures(1)
    volts1 = self.adc.read()
    print 'Volts: ', volts1
    self.adc.set_exposures(2)
    volts2 = self.adc.read()
    print 'Volts: ', volts2

    for v1, v2 in zip(volts1, volts2):
      assert abs(v2) > abs(v1)

  def test_outputs(self):
    self._banner('Digital Output Test')
    for pin in xrange(8):
      for ch in xrange(4):
        e = raw_input('Connect D%d to channel %d, Enter E to end. '%((49-pin), ch+1))
        if e == 'E':
          return
        self.adc.output_off(pin)
        voltsoff = self.adc.read()

        self.adc.output_on(pin)
        voltson = self.adc.read()

        print 'on=',voltson[ch], 'off=',voltsoff[ch]

        assert voltson[ch] > voltsoff[ch]

  def test_trigger(self):
    self._banner('Trigger Test')
    period_ms = 2
    halfperiod_s = period_ms*0.5/1000

    e = raw_input('Connect D49 to D38, enter E to end.')
    if e == 'E':
      return

    ntriggers = 200

    for nchannels in (1,2,3,4):
      for trigger_div in (1,2,3,5):
        self.adc.set_trigger_divider(trigger_div)
        self.adc.scan(nchannels)

        assert self.adc.buffer_writepos == 0

        for x in xrange(ntriggers):
          self.adc.output_on(0)
          time.sleep(halfperiod_s)
          self.adc.output_off(0)
          time.sleep(halfperiod_s)
          #print self.adc.buffer_writepos, x+1

        writepos = self.adc.buffer_writepos
        print 'write pos at %d after %d scans of %d channels'%(writepos, ntriggers/trigger_div, nchannels)
        assert writepos == ntriggers//trigger_div * nchannels

if __name__ == '__main__':
  import nose
  nose.main()
