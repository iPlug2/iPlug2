#pragma once

typedef double sampleType;

template <typename sampleType>
class IOscillator
{
public:
  IOscillator(double startPhase = 0.)
  : mStartPhase(startPhase)
  {
    SetFreqCPS(1.);
  }
  
  virtual inline sampleType Process(double freqHz)
  {
    SetFreqCPS(freqHz);
    mPhase = mPhase + mPhaseIncr;
    return std::sin(mPhase * PI * 2.);
  }

  inline void SetFreqCPS(double freqHz)
  {
    mPhaseIncr = mSampleRateReciprocal * freqHz;
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRateReciprocal = (1./sampleRate);
  }

  void Reset()
  {
    mPhase = 0.;
  }
  
protected:
  double mPhase = 0.;  // float phase (goes between 0. and 1.)
  double mPhaseIncr = 0.; // how much to add to the phase on each sampleType
  double mSampleRateReciprocal = 1./44100.;
  double mStartPhase;
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

template <typename sampleType>
class FastSinOscillator : public IOscillator<sampleType>
{
  union tabfudge
  {
    double d;
    int i[2];
  } ALIGNED(8);

public:
  FastSinOscillator(double startPhase = 0.)
  : IOscillator<sampleType>(startPhase)
  {
  }
  
  //todo rewrite this
  inline sampleType Process(double freqCPS) override
  {
    IOscillator<sampleType>::SetFreqCPS(freqCPS);
    
    double output = 0.;
    ProcessBlock(&output, 1);
    
    return output;
  }
  
  void ProcessBlock(sampleType* pOutput, int nFrames)
  {
    double phase = IOscillator<sampleType>::mPhase + (double) UNITBIT32;
    const double phaseIncr = IOscillator<sampleType>::mPhaseIncr * tableSize;

    union tabfudge tf;
    tf.d = UNITBIT32;
    const int normhipart = tf.i[HIOFFSET];

    for (int s = 0; s < nFrames; s++)
    {
      tf.d = phase;
      phase += phaseIncr;
      const sampleType* addr = mLUT + (tf.i[HIOFFSET] & tableSizeM1); // Obtain the integer portion
      tf.i[HIOFFSET] = normhipart; // Force the double to wrap.
      const double frac = tf.d - UNITBIT32;
      const sampleType f1 = addr[0];
      const sampleType f2 = addr[1];
      pOutput[s] = sampleType(f1 + frac * (f2 - f1));
    }

    // Restore mPhase
    tf.d = UNITBIT32 * tableSize;
    const int normhipart2 = tf.i[HIOFFSET];
    tf.d = phase + (UNITBIT32 * tableSize - UNITBIT32); // Remove the offset we introduced at the start of UNITBIT32.
    tf.i[HIOFFSET] = normhipart2;
    IOscillator<sampleType>::mPhase = tf.d - UNITBIT32 * tableSize;
  }

private:
  static const int tableSize = 512; // 2^9
  static const int tableSizeM1 = 511; // 2^9 -1
  static const sampleType mLUT[513];
} ALIGNED(8);

#include "IPlugOscillator_table.h"
