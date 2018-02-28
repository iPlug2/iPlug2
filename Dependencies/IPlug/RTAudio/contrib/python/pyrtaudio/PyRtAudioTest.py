
import rtaudio as rt

from math import cos

import struct


class audio_generator:
    def __init__(self):
        self.idx = -1
        self.freq = 440.
    def __call__(self):
	self.idx += 1
	if self.idx%48000 == 0:
            self.freq *= 2**(1/12.)	
        return 0.5*cos(2.*3.1416*self.freq*self.idx/48000.)


class callback:
    def __init__(self, gen):
        self.gen = gen
	self.i = 0
    def __call__(self,playback, capture):
        [struct.pack_into("f", playback, 4*o, self.gen()) for o in xrange(256)]
	self.i = self.i + 256
	if self.i > 48000*10:
            print '.'
            return 1

dac = rt.RtAudio()

n = dac.getDeviceCount()
print 'Number of devices available: ', n

for i in range(n):
    try:
        print dac.getDeviceInfo(i)
    except rt.RtError as e:
	print e


print 'Default output device: ', dac.getDefaultOutputDevice()
print 'Default input device: ', dac.getDefaultInputDevice()

print 'is stream open: ', dac.isStreamOpen()
print 'is stream running: ', dac.isStreamRunning()

oParams = {'deviceId': 1, 'nChannels': 1, 'firstChannel': 0}
iParams = {'deviceId': 1, 'nChannels': 1, 'firstChannel': 0}

try:
  dac.openStream(oParams,oParams,48000,256,callback(audio_generator()) )
except rt.RtError as e:
  print e
else:
  dac.startStream()

  import time
  print 'latency: ', dac.getStreamLatency()

  while (dac.isStreamRunning()):
    time.sleep(0.1)

  print dac.getStreamTime()

  dac.stopStream()
  dac.abortStream()
  dac.closeStream()

