/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "denormal.h"
#include "IPlugConstants.h"

BEGIN_IPLUG_NAMESPACE

template<typename T, int NC = 1>
class LogParamSmooth
{
private:
  double mA, mB;
  T mOutM1[NC];

public:
  LogParamSmooth(double timeMs = 5., T initalValue = 0.)
  {
    for (auto i = 0; i < NC; i++)
    {
      mOutM1[i] = initalValue;
    }
    
    SetSmoothTime(2., DEFAULT_SAMPLE_RATE);
  }

  // only works for NC = 1
  inline T Process(T input)
  {
    mOutM1[0] = (input * mB) + (mOutM1[0] * mA);
    denormalFix(&mOutM1[0]);
    return mOutM1[0];
  }

  inline void SetValue(T value)
  {
    for (auto i = 0; i < NC; i++)
    {
      mOutM1[i] = value;
    }
  }

  inline void SetValues(T values[NC])
  {
    for (auto i = 0; i < NC; i++)
    {
      mOutM1[i] = values[i];
    }
  }

  void SetSmoothTime(double timeMs, double sampleRate)
  {
    static constexpr double TWO_PI = 6.283185307179586476925286766559;
    
    mA = exp(-TWO_PI / (timeMs * 0.001 * sampleRate));
    mB = 1.0 - mA;
  }

  void ProcessBlock(T inputs[NC], T** outputs, int nFrames, int channelOffset = 0)
  {
    const T b = mB;
    const T a = mA;

    for (auto s = 0; s < nFrames; ++s)
    {
      for (auto c = 0; c < NC; c++)
      {
        T output = (inputs[channelOffset + c] * b) + (mOutM1[c] * a);
        denormal_fix(&output);
        mOutM1[c] = output;
        outputs[channelOffset + c][s] = output;
      }
    }
  }

} WDL_FIXALIGN;

END_IPLUG_NAMESPACE
