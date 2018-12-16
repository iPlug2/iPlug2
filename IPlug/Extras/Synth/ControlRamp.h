/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#pragma once

/**
 * @file
 * @copydoc ControlRamp
 */

#include <iostream>

/** A ControlRamp describes one value changing over time. It can
 * be easily converted into a signal for more processing,
 * or if sample accuracy is not needed, just the end value can be used.
 * It describes a piecewise function in three pieces:
 * from [0, startValue] to [transitionStart, startValue]
 * from [transitionStart, startValue] to [transitionEnd, endValue]
 * from [transitionEnd, endValue] to [blockSize, endValue]
 */
struct ControlRamp
{
public:
  double startValue;
  double endValue;
  int transitionStart;
  int transitionEnd;

  void Clear()
  {
    startValue = endValue = 0.;
    transitionStart = transitionEnd = 0;
  }

  bool IsNonzero() const
  {
    return (startValue != 0.) || (endValue != 0.);
  }

  /** Writes the ramp signal to an output buffer.
   * @param buffer Pointer to the start of an output buffer.
   * @param startIdx Sample index of the start of the desired write within the buffer.
   * @param nFrames The number of samples to be written. */
  void Write(float* buffer, int startIdx, int nFrames)
  {
    float val = startValue;
    float dv = (endValue - startValue)/(transitionEnd - transitionStart);
    for(int i=startIdx; i<startIdx + transitionStart; ++i)
    {
      buffer[i] = val;
    }
    for(int i=startIdx + transitionStart; i<startIdx + transitionEnd; ++i)
    {
      val += dv;
      buffer[i] = val;
    }
    for(int i=startIdx + transitionEnd; i<startIdx + nFrames; ++i)
    {
      buffer[i] = val;
    }
  }
};

struct ControlRampProcessor
{
public:
  // process the glide and write changes to the output ramp.
  void Process(int blockSize)
  {
    // always connect with previous block
    mpOutput->startValue = mpOutput->endValue;

    if(mSamplesRemaining)
    {
      if(mSamplesRemaining == mGlideSamples)
      {
        // start glide
        if(mSamplesRemaining > blockSize)
        {
          // start with ramp to block end
          int glideStartSamples = blockSize - mStartOffset;
          mpOutput->endValue = mpOutput->startValue + glideStartSamples*mChangePerSample;
          mpOutput->transitionStart = mStartOffset;
          mpOutput->transitionEnd = blockSize;
          mSamplesRemaining -= glideStartSamples;
        }
        else
        {
          // glide starts and finishes within block
          mpOutput->endValue = mTargetValue;
          mpOutput->transitionStart = mStartOffset;
          mpOutput->transitionEnd = mStartOffset + mGlideSamples;
          mSamplesRemaining = 0;
        }
      }
      else if(mSamplesRemaining > blockSize)
      {
        // continue glide
        mpOutput->endValue = mpOutput->startValue + mChangePerSample*blockSize;
        mpOutput->transitionStart = 0;
        mpOutput->transitionEnd = blockSize;
        mSamplesRemaining -= blockSize;
      }
      else
      {
        // finish glide
        mpOutput->endValue = mTargetValue;
        mpOutput->transitionStart = 0;
        mpOutput->transitionEnd = mSamplesRemaining;
        mSamplesRemaining = 0;
      }
    }
  }

  // set the next target for the glide without writing directly to the ramp.
  void SetTarget(double targetValue, int startOffset, int glideSamples, int blockSize)
  {
    mTargetValue = targetValue;
    if(glideSamples < 1) glideSamples = 1;
    mGlideSamples = glideSamples;
    mSamplesRemaining = glideSamples;
    mChangePerSample = (targetValue - mpOutput->endValue)/glideSamples;
    mStartOffset = startOffset;
  }

  ControlRamp* mpOutput {nullptr};
  double mTargetValue {0.};
  double mChangePerSample {0.};
  int mGlideSamples {0};
  int mSamplesRemaining {0};
  int mStartOffset {0};
};
