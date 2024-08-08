
from __future__ import print_function
import threading
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
        [struct.pack_into("f", playback, 4*o, self.gen()) for o in range(256)]
        self.i = self.i + 256
        if self.i > 48000*10:
            print('.')
            return 1

try:
    # if we have numpy, replace the above class
    import numpy as np
    class callback:
        def __init__(self, gen):
            print('Using Numpy.')
            self.freq = 440.
            t = np.arange(256, dtype=np.float32) / 48000.0
            self.phase = 2*np.pi*t
            self.inc = 2*np.pi*256/48000
            self.k = 0
        def __call__(self, playback, capture):
            # Calculate sinusoid using numpy vector operation, as
            # opposed to per-sample computations in the generator
            # above that must be collected and packed one at a time.
            self.k += 256
            if self.k > 48000:
                self.freq *= 2**(1/12.)
                self.k = 0
            self.phase += self.inc
            samples = 0.5*np.cos(self.phase * self.freq)

            # Ensure result is the right size!
            assert samples.shape[0] == 256
            assert samples.dtype == np.float32

            # Use numpy array view to do a once-copy into memoryview
            # (ie. we only do a single byte-wise copy of the final
            # result into 'playback')
            usamples = samples.view(dtype=np.uint8)
            playback_array = np.array(playback, copy=False)
            np.copyto(playback_array, usamples)
except ModuleNotFoundError:
    print('Numpy not available, using struct.')

dac = rt.RtAudio()

n = dac.getDeviceCount()
print('Number of devices available: ', n)

for i in range(n):
    try:
        print(dac.getDeviceInfo(i))
    except rt.RtError as e:
        print(e)


print('Default output device: ', dac.getDefaultOutputDevice())
print('Default input device: ', dac.getDefaultInputDevice())

print('is stream open: ', dac.isStreamOpen())
print('is stream running: ', dac.isStreamRunning())

oParams = {'deviceId': 0, 'nChannels': 1, 'firstChannel': 0}
iParams = {'deviceId': 0, 'nChannels': 1, 'firstChannel': 0}

try:
  dac.openStream(oParams,oParams,48000,256,callback(audio_generator()) )
except rt.RtError as e:
  print(e)
else:
  dac.startStream()

  import time
  print('latency: ', dac.getStreamLatency())

  while (dac.isStreamRunning()):
    time.sleep(0.1)

  print(dac.getStreamTime())

  dac.stopStream()
  dac.abortStream()
  dac.closeStream()
