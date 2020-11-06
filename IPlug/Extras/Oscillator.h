/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

template <typename T>
class IOscillator
{
public:
  IOscillator(double startPhase = 0., double startFreq = 1.)
  : mStartPhase(startPhase)
  {
    SetFreqCPS(startFreq);
  }

  virtual inline T Process(double freqHz) = 0;

  inline void SetFreqCPS(double freqHz)
  {
    mPhaseIncr = (1./mSampleRate) * freqHz;
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
  }

  void Reset()
  {
    mPhase = mStartPhase;
  }
  
  void SetPhase(double phase)
  {
    mPhase = phase;
  }

protected:
  double mPhase = 0.;  // float phase (goes between 0. and 1.)
  double mPhaseIncr = 0.; // how much to add to the phase on each T
  double mSampleRate = 44100.;
  double mStartPhase;
};

template <typename T>
class SinOscillator : public IOscillator<T>
{
public:
  SinOscillator(double startPhase = 0., double startFreq = 1.)
  : IOscillator<T>(startPhase, startFreq)
  {
  }
  
  inline T Process()
  {
    IOscillator<T>::mPhase = IOscillator<T>::mPhase + IOscillator<T>::mPhaseIncr;
    return std::sin(IOscillator<T>::mPhase * PI * 2.);
  }

  inline T Process(double freqHz) override
  {
    IOscillator<T>::SetFreqCPS(freqHz);
    IOscillator<T>::mPhase = IOscillator<T>::mPhase + IOscillator<T>::mPhaseIncr;
    return std::sin(IOscillator<T>::mPhase * PI * 2.);
  }
};

/*
 FastSinOscillator - fast sinusoidal oscillator / table look up, based on an approach and code used by Miller Puckette in Pure Data, originally by Robert Höldrich

 From some correspondence with M.S.P...

 The basic idea is this: if you have a double precision floating point number
 between -2^19 (-524288)  and +2^19 (+524288), and you want the fractional part as an
 integer, proceed as follows:

 Add 3*2^19 (1572864) to it.  The sum will be between 2^20 (1048576) and 2^21 (2097152) and so, in the usual
 double precision format, the units bit will be the LSB of the higher-order
 32 bits, and the fractional part will be neatly represented as an unsigned
 fixed-point number between 0 and 2^32-1.

 You can then use a C union to read these lower 32 bits as an integer.  Or, if
 you want to access then as a floating-point number without having to use
 fix-to-float conversion, just bash the upper 32 bits to their original
 state for representing 3*2^19.  So far this is what's going on in the wrap~
 and phasor~ objects in Pd.

 The objects that do interpolating table lookup (e.g., cos~) take this one step
 further.  Here you end up with a number (now supposing it to be between
 -2^10 and +2^10).  Multiply by 2^9 (the size of the table) so that the
 9-bit table address is in the 9 lowest bits of the upper 32 bit 'word', and
 the fractional part is in the lower 32 bits as before.  Grab the address as
 an integer (masking to get just the 9 bits we want).

 The you need to re-convert the fractional address into floating-point format
 so you can use it to interpolate.  Just bash the upper 32 bits with the
 original value for 3*2^19, and the result will be a 64-bit floating-point number
 between 3*2^19 and 3*2^19+1.  Subtracting 3*2^19 from this gives us a floating-
 point representation of the fractional part.
 */

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */
#define HIOFFSET 1
#define LOWOFFSET 0

#ifdef _MSC_VER
#define ALIGN16 __declspec(align(16))
#define ALIGNED(x)
#else
#define ALIGN16 alignas(16)
#define ALIGNED(x) __attribute__ ((aligned (x)))
#endif

template <typename T>
class FastSinOscillator : public IOscillator<T>
{
  union tabfudge
  {
    double d;
    int i[2];
  } ALIGNED(8);

public:
  FastSinOscillator(double startPhase = 0., double startFreq = 1.)
  : IOscillator<T>(startPhase * tableSizeM1, startFreq)
  {
  }

  //todo rewrite this
  inline T Process()
  {
    T output = 0.;
    ProcessBlock(&output, 1);

    return output;
  }

  inline T Process(double freqCPS) override
  {
    IOscillator<T>::SetFreqCPS(freqCPS);

    T output = 0.;
    ProcessBlock(&output, 1);

    return output;
  }

  static inline T Lookup(double phaseRadians)
  {
    double tPhase = phaseRadians / (PI * 2.) * tableSizeM1;

    tPhase += (double) UNITBIT32;

    union tabfudge tf;
    tf.d = UNITBIT32;
    const int normhipart = tf.i[HIOFFSET];

    tf.d = tPhase;
    const T* addr = mLUT + (tf.i[HIOFFSET] & tableSizeM1);
    tf.i[HIOFFSET] = normhipart;
    const double frac = tf.d - UNITBIT32;
    const T f1 = addr[0];
    const T f2 = addr[1];
    return f1 + frac * (f2 - f1);
  }

  void ProcessBlock(T* pOutput, int nFrames)
  {
    double phase = IOscillator<T>::mPhase + (double) UNITBIT32;
    const double phaseIncr = IOscillator<T>::mPhaseIncr * tableSize;

    union tabfudge tf;
    tf.d = UNITBIT32;
    const int normhipart = tf.i[HIOFFSET];

    for (auto s = 0; s < nFrames; s++)
    {
      tf.d = phase;
      phase += phaseIncr;
      const T* addr = mLUT + (tf.i[HIOFFSET] & tableSizeM1); // Obtain the integer portion
      tf.i[HIOFFSET] = normhipart; // Force the double to wrap.
      const double frac = tf.d - UNITBIT32;
      const T f1 = addr[0];
      const T f2 = addr[1];
      mLastOutput = pOutput[s] = T(f1 + frac * (f2 - f1));
    }

    // Restore mPhase
    tf.d = UNITBIT32 * tableSize;
    const int normhipart2 = tf.i[HIOFFSET];
    tf.d = phase + (UNITBIT32 * tableSize - UNITBIT32); // Remove the offset we introduced at the start of UNITBIT32.
    tf.i[HIOFFSET] = normhipart2;
    IOscillator<T>::mPhase = tf.d - UNITBIT32 * tableSize;
  }

  T mLastOutput = 0.;
private:
  static const int tableSize = 512; // 2^9
  static const int tableSizeM1 = 511; // 2^9 -1
  static const T mLUT[513];
} ALIGNED(8);

#include "Oscillator_table.h"

END_IPLUG_NAMESPACE
