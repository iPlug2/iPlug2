/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 The class in this file is derived from the FAUST ef.gate_mono code
 
 https://faustlibraries.grame.fr/libs/misceffects/#efgate_mono

 analyzers.lib/amp_follower_ar:author Jonatan Liljedahl, revised by Romain Michon
 signals.lib/onePoleSwitching:author Jonatan Liljedahl, revised by Dario Sanfilippo
 licence STK-4.3
 
 https://github.com/grame-cncm/faustlibraries/blob/master/licenses/stk-4.3.0.md
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Multichannel NoiseGate
 */

#pragma once

#include "IPlugUtilities.h"

BEGIN_IPLUG_NAMESPACE

template<typename T, int NC = 1>
class NoiseGate
{
public:
  void SetThreshold(double thresholdDB)
  {
    mThreshold = DBToAmp(thresholdDB);
  }
  
  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
  }
  
  void SetAttackTime(double attackTime)
  {
    mAttackTime = std::max(attackTime, (1.0 / mSampleRate));
    mMinRate = std::min(mAttackTime, mReleaseTime);
  }
  
  void SetHoldTime(double holdTime)
  {
    mHoldTime = std::max(holdTime, (1.0 / mSampleRate));
  }
  
  void SetReleaseTime(double releaseTime)
  {
    mReleaseTime = std::max(releaseTime, (1.0 / mSampleRate));
    mMinRate = std::min(mAttackTime, mReleaseTime);
  }

  void ProcessBlock(T** inputs, T** outputs, T* sidechain, int nChans, int nFrames)
  {
    assert(nChans <= NC);

    for (auto s=0; s <nFrames; s++)
    {
      auto trigger = sidechain[s];
      auto gain = processSample(trigger);
      for (auto c=0; c<nChans; c++)
      {
        outputs[c][s] = inputs[c][s] * gain;
      }
    }
  }

private:
  inline T processSample(T sample) 
  {
    auto ampFollower = [](T x, T& y, double att, double rel, double sr) {
      auto tau2pole = [](double tau, double sr) {
        return std::exp(-1.0 / (tau * sr));
      };
      const auto absX = std::abs(x);
      const auto coeff = (absX > y) ? T(tau2pole(att, sr)) : T(tau2pole(rel, sr));
      y = (T(1) - coeff) * absX + coeff * y;
      return y;
    };
    
    auto rawGate = ampFollower(sample, mHistory2, mMinRate, mMinRate, mSampleRate) > mThreshold;
    
    if (rawGate < mPrevGate)
    {
      mHoldCounter = int(mHoldTime * mSampleRate);
    }
    else 
    {
      if (mHoldCounter > 0)
      {
        mHoldCounter--;
      }
    }
    
    const auto heldGate = std::max(rawGate, mHoldCounter > 0);
    const auto smoothed = ampFollower(T(heldGate), mHistory1, mAttackTime, mReleaseTime, mSampleRate);

    mPrevGate = rawGate;
    
    return smoothed;
  }

  double mThreshold = 0.5;
  double mSampleRate = 48000.0;
  double mAttackTime = 0.01f;
  double mHoldTime = 0.01f;
  double mReleaseTime = 0.01f;
  double mMinRate = mAttackTime;
  T mHistory1, mHistory2 = 0.0f;
  bool mPrevGate = false;
  int mHoldCounter = 0;
};

END_IPLUG_NAMESPACE
