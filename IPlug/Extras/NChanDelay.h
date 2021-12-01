/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once
#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

// A static delayline used to delay bypassed signals to match mLatency in AAX/VST3/AU
template<typename T>
class NChanDelayLine
{
public:
  NChanDelayLine(int nInputChans = 2, int nOutputChans = 2)
  : mNInChans(nInputChans)
  , mNOutChans(nOutputChans)
  {}

  void SetDelayTime(int delayTimeSamples)
  {
    mDTSamples = delayTimeSamples;
    mBuffer.Resize(mNInChans * delayTimeSamples);
    mWriteAddress = 0;
    ClearBuffer();
  }

  void ClearBuffer()
  {
    memset(mBuffer.Get(), 0, mNInChans * mDTSamples * sizeof(T));
  }

  void ProcessBlock(T** inputs, T** outputs, int nFrames)
  {
    T* buffer = mBuffer.Get();

    for (auto s = 0 ; s < nFrames; ++s)
    {
      for (auto c = 0; c < mNInChans; c++)
      {
        if (c < mNOutChans)
        {
          double input = inputs[c][s];
          const int offset = c * mDTSamples;
          outputs[c][s] = buffer[offset + mWriteAddress];
          buffer[offset + mWriteAddress] = input;
        }
      }

      mWriteAddress++;
      mWriteAddress %= mDTSamples;
    }
  }

private:
  WDL_TypedBuf<T> mBuffer;
  uint32_t mNInChans, mNOutChans;
  uint32_t mWriteAddress = 0;
  uint32_t mDTSamples = 0;
} WDL_FIXALIGN;

END_IPLUG_NAMESPACE
