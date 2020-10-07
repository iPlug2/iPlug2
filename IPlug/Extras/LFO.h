/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Basic unoptimized tempo-syncable LFO implementation
 */

#include "Oscillator.h"

BEGIN_IPLUG_NAMESPACE

#define LFO_TEMPODIV_VALIST "1/64", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2", "1/1", "2/1", "4/1", "8/1"

#define LFO_SHAPE_VALIST "Triangle", "Square", "Ramp Up", "Ramp Down"

template<typename T = double>
class LFO : public IOscillator<T>
{
public:
  enum ETempoDivison
  {
    k64th = 0,   // 1 sixty fourth of a beat
    k32nd,       // 1 thirty second of a beat
    k16thT,      // 1 sixteenth note triplet
    k16th,       // 1 sixteenth note
    k16thD,      // 1 dotted sixteenth note
    k8thT,       // 1 eighth note triplet
    k8th,        // 1 eighth note
    k8thD,       // 1 dotted eighth note
    k4th,        // 1 quater note a.k.a 1 beat @ 4/4
    k4thD,       // 1 dotted beat @ 4/4
    k2th,        // 2 beats @ 4/4
    k1,          // 1 bar @ 4/4
    k2,          // 2 bars @ 4/4
    k4,          // 4 bars @ 4/4
    k8,          // 8 bars @ 4/4
    kNumDivisions
  };

  enum EShape
  {
    kTriangle,
    kSquare,
    kRampUp,
    kRampDown,
//    kSine,
    kNumShapes
  };
  
  enum class EPolarity
  {
    kUnipolar,
    kBipolar
  };
  
  enum class ERateMode
  {
    kHz,
    kBPM
  };

  /** Get the scalar factor to convert a ramp at the host BPM to tempo division value */
  static T GetQNScalar(ETempoDivison division)
  {
    static constexpr T scalars[kNumDivisions] = {
      64.   / 4.,
      32.   / 4.,
      24.   / 4.,
      16.   / 4.,
      12.   / 4.,
      9.    / 4.,
      8.    / 4.,
      6     / 4.,
      4.    / 4.,
      3.    / 4.,
      2.    / 4.,
      1.    / 4.,
      0.5   / 4.,
      0.25  / 4.,
      0.125 / 4.
    };
    
    return scalars[division];
  }
  
  /** Get a CString to display the divisor as text */
  static const char* GetQNDisplay(ETempoDivison division)
  {
    static const char* displays[kNumDivisions] = { LFO_TEMPODIV_VALIST };
    return displays[division];
  }

  /** Per sample process function, updating frequency per sample */
  inline T Process(double freqHz) override
  {
    IOscillator<T>::SetFreqCPS(freqHz);
    IOscillator<T>::mPhase = WrapPhase(IOscillator<T>::mPhase + IOscillator<T>::mPhaseIncr);
    
    return DoProcess(IOscillator<T>::mPhase);
  }

  /* Block process function */
  void ProcessBlock(T* pOutput, int nFrames, double qnPos = 0., bool transportIsRunning = false, double tempo = 120.)
  {
    T oneOverQNScalar = 1./mQNScalar;
    T phase = IOscillator<T>::mPhase;
    
    if(mRateMode == ERateMode::kBPM && !transportIsRunning)
      IOscillator<T>::SetFreqCPS(tempo/60.);
    
    T phaseIncr = IOscillator<T>::mPhaseIncr;

    for (int s=0; s<nFrames; s++)
    {
      if(mRateMode == ERateMode::kBPM)
      {
        if(transportIsRunning)
          phase = std::fmod(qnPos, oneOverQNScalar) / oneOverQNScalar;
        else
          phase = WrapPhase(phase + (phaseIncr * mQNScalar));
      }
      else
        phase = WrapPhase(phase + phaseIncr);
      
      pOutput[s] = DoProcess(phase);
    }
    
    IOscillator<T>::mPhase = phase;
  }
  
  void SetShape(int lfoShape)
  {
    mShape = (EShape) Clip(lfoShape, 0, kNumShapes-1);
  }
  
  void SetPolarity(bool bipolar)
  {
    mPolarity = bipolar ? EPolarity::kBipolar : EPolarity::kUnipolar;
  }
  
  void SetScalar(T scalar)
  {
    mLevelScalar = scalar;
  }
  
  void SetQNScalar(T scalar)
  {
    mQNScalar = scalar;
  }
  
  void SetQNScalarFromDivision(int division)
  {
    mQNScalar = GetQNScalar(static_cast<ETempoDivison>(Clip(division, 0, (int) kNumDivisions)));
  }
  
  void SetRateMode(bool sync)
  {
    mRateMode = sync ? ERateMode::kBPM : ERateMode::kHz;
  }
  
  T GetLastOutput() const
  {
    return mLastOutput;
  }
  
private:
  static inline T WrapPhase (T x, T lo = 0., T hi = 1.)
  {
    while (x >= hi)
      x -= hi;
    while (x < lo)
      x += hi - lo;
    return x;
  };
  
  inline T DoProcess(T phase)
  {
    auto triangle         = [](T x){ return (2. * (1. - std::abs((WrapPhase(x + 0.25) * 2.) -1.))) - 1.; };
    auto triangleUnipolar = [](T x){ return 1. - std::abs((x * 2.) - 1. ); };
    auto square           = [](T x){ return std::copysign(1., x - 0.5); };
    auto squareUnipolar   = [](T x){ return std::copysign(0.5, x - 0.5) + 0.5; };
    auto rampup           = [](T x){ return (x * 2.) - 1.; };
    auto rampupUnipolar   = [](T x){ return x; };
    auto rampdown         = [](T x){ return ((1. - x) * 2.) - 1.; };
    auto rampdownUnipolar = [](T x){ return 1. - x; };
    
    T output = 0.;
    
    if(mPolarity == EPolarity::kUnipolar)
    {
      switch (mShape) {
        case kTriangle: output = triangleUnipolar(phase); break;
        case kSquare:   output = squareUnipolar(phase); break;
        case kRampUp:   output = rampupUnipolar(phase); break;
        case kRampDown: output = rampdownUnipolar(phase); break;
        default: break;
      }
    }
    else
    {
      switch (mShape) {
        case kTriangle: output = triangle(phase); break;
        case kSquare:   output = square(phase); break;
        case kRampUp:   output = rampup(phase); break;
        case kRampDown: output = rampdown(phase); break;
        default: break;
      }
    }
    
    mLastOutput = output * mLevelScalar;
    
    return mLastOutput;
  }

private:
  T mLastOutput = 0.;
  T mLevelScalar = 1.; // Non clipped, or smoothed scalar value
  T mQNScalar = 1.;
  EShape mShape = EShape::kTriangle;
  EPolarity mPolarity = EPolarity::kUnipolar;
  ERateMode mRateMode = ERateMode::kHz;
};

END_IPLUG_NAMESPACE
