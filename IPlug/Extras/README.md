# IPlug Extras

In this folder there are a collection of DSP classes to facilitate plug-in development. The implementations here are not necessarily highly optimised.

* **ADSR:** a basic ADSR Envelope generator 
* **MidiSynth:** a monophonic/polyphonic MPE capable synthesiser base class which can be supplied with a custom voice
* **OverSampler:** a class for performing up 16x oversampling of a signal.
* **Oscillator:** an oscillator base class and inheriting classes. Includes a fast sinusoidal table lookup oscillator
* **LFO:** unoptimized tempo-syncable LFO
* **SVF:** a multichannel state variable filter for basic EQing
* **NChanDelay:** a multichannel delay line (delays all channels by the same amount)
* **WebSocket:**  classes for remote controlling a plug-in over web sockets
