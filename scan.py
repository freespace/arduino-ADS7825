import time
from ads7825 import ADS7825

def main(serialport, nscans, nchannels):
  adc = ADS7825(serialport)
  adc.scan()

  done = False
  while not done:
    time.sleep(3)
    try:
      data = adc.read_scan(nscans, nchannels=nchannels)
      done = True
    except Exception, ex:
      print ex
      
  print len(data)

if __name__ == '__main__':
  import sys
  if len(sys.argv) != 4:
    print sys.argv[0], '<serial port> <nscans> <nchannels>'
    sys.exit(1)
  else:
    sys.exit(main(*sys.argv[1:]))

