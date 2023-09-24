/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include <functional>
#include <cmath>

#include "IPlugPlatform.h"

#include "heapbuf.h"
#include "ptrlist.h"

#include "LanczosResampler.h"

BEGIN_IPLUG_NAMESPACE

enum ESRCMode
{
  kLinearInterpolation = 0,
  kCubicInterpolation,
  kLancsoz,
  kNumResamplingModes
};

template<typename T = double, int NCHANS=2>
class NonIntegerResampler
{
public:
  using BlockProcessFunc = std::function<void(T**, T**, int)>;
  
  NonIntegerResampler(double renderingSampleRate, ESRCMode mode = ESRCMode::kLinearInterpolation)
  : mResamplingMode(mode)
  , mRenderingSampleRate(renderingSampleRate)
  {
  }
  
  ~NonIntegerResampler()
  {
  }

  NonIntegerResampler(const NonIntegerResampler&) = delete;
  NonIntegerResampler& operator=(const NonIntegerResampler&) = delete;

  void SetResamplingMode(ESRCMode mode)
  {
    mResamplingMode = mode;
    Reset(mInputSampleRate);
  }
  
  void Reset(double inputSampleRate, int blockSize = DEFAULT_BLOCK_SIZE)
  {
    mInputSampleRate = inputSampleRate;
    mUpRatio = mInputSampleRate / mRenderingSampleRate;
    mDownRatio = mRenderingSampleRate / mInputSampleRate;
    mResampledData.Resize(DEFAULT_BLOCK_SIZE * NCHANS);
    memset(mResampledData.Get(), 0.0f, DEFAULT_BLOCK_SIZE * NCHANS * sizeof(T));
    mScratchPtrs.Empty();
    
    for (auto chan=0; chan<NCHANS; chan++)
    {
      mScratchPtrs.Add(mResampledData.Get() + (chan * DEFAULT_BLOCK_SIZE));
    }

    if (mResamplingMode == ESRCMode::kLancsoz)
    {
      mResamplerUp = std::make_unique<LanczosResampler<T, NCHANS>>(mInputSampleRate, mRenderingSampleRate);
      mResamplerDown = std::make_unique<LanczosResampler<T, NCHANS>>(mRenderingSampleRate, mInputSampleRate);
        
      /* Prepopulate the upsampler with silence so it can run ahead */
      auto numSamplesToAdvance = mResamplerUp->inputsRequiredToGenerateOutputs(1) * 2;
      mResamplerUp->pushBlock(mScratchPtrs.GetList(), numSamplesToAdvance);
    }
  }

  /** Resample an input block with a per-block function (up sample input -> process with function -> down sample)
   * @param inputs Two-dimensional array containing the non-interleaved input buffers of audio samples for all channels
   * @param outputs Two-dimensional array for audio output (non-interleaved).
   * @param nFrames The block size for this block: number of samples per channel.
   * @param func The function that processes the audio sample at the higher sampling rate. NOTE: std::function can call malloc if you pass in captures */
  void ProcessBlock(T** inputs, T** outputs, int nFrames, BlockProcessFunc func)
  {
    switch (mResamplingMode) 
    {
      case ESRCMode::kLinearInterpolation:
      {
        const auto nNewFrames = LinearInterpolate(inputs, mScratchPtrs.GetList(), nFrames, mUpRatio, DEFAULT_BLOCK_SIZE);
        func(mScratchPtrs.GetList(), mScratchPtrs.GetList(), nNewFrames);
        LinearInterpolate(mScratchPtrs.GetList(), outputs, nNewFrames, mDownRatio, nFrames);
        break;
      }
      case ESRCMode::kCubicInterpolation:
      {
        const auto nNewFrames = CubicInterpolate(inputs, mScratchPtrs.GetList(), nFrames, mUpRatio, DEFAULT_BLOCK_SIZE);
        func(mScratchPtrs.GetList(), mScratchPtrs.GetList(), nNewFrames);
        CubicInterpolate(mScratchPtrs.GetList(), outputs, nNewFrames, mDownRatio, nFrames);
        break;
      }
      case ESRCMode::kLancsoz:
      {
        mResamplerUp->pushBlock(inputs, nFrames);
        
        const auto outputLen = static_cast<int>(std::ceil(static_cast<double>(nFrames) / mUpRatio));

        while (mResamplerUp->inputsRequiredToGenerateOutputs(outputLen) == 0)
        {
          mResamplerUp->popBlock(mScratchPtrs.GetList(), outputLen);
          func(mScratchPtrs.GetList(), mScratchPtrs.GetList(), outputLen);
          
          mResamplerDown->pushBlock(mScratchPtrs.GetList(), outputLen);
        }
        
        mResamplerDown->popBlock(outputs, nFrames);
        mResamplerUp->renormalizePhases();
        mResamplerDown->renormalizePhases();
        break;
      }
      default:
        break;
    }
  }

private:
  static inline int LinearInterpolate(T** inputs, T** outputs, int inputLen, double ratio, int maxOutputLen)
  {
    const auto outputLen =
      std::min(static_cast<int>(std::ceil(static_cast<double>(inputLen) / ratio)), maxOutputLen);

    for (auto writePos = 0; writePos < outputLen; writePos++)
    {
      const auto readPos = ratio * static_cast<double>(writePos);
      const auto readPostionTrunc = std::floor(readPos);
      const auto readPosInt = static_cast<int>(readPostionTrunc);

      if (readPosInt < inputLen)
      {
        const auto y = readPos - readPostionTrunc;

        for (auto chan=0; chan<NCHANS; chan++)
        {
          const auto x0 = inputs[chan][readPosInt];
          const auto x1 = ((readPosInt + 1) < inputLen) ? inputs[chan][readPosInt + 1] : inputs[chan][readPosInt-1];
          outputs[chan][writePos] = (1.0 - y) * x0 + y * x1;
        }
      }
    }

    return outputLen;
  }
  
  static inline int CubicInterpolate(T** inputs, T** outputs, int inputLen, double ratio, int maxOutputLen)
  {
    const auto outputLen =
      std::min(static_cast<int>(std::ceil(static_cast<double>(inputLen) / ratio)), maxOutputLen);

    for (auto writePos = 0; writePos < outputLen; writePos++)
    {
      const auto readPos = ratio * static_cast<double>(writePos);
      const auto readPostionTrunc = std::floor(readPos);
      const auto readPosInt = static_cast<int>(readPostionTrunc);

      if (readPosInt < inputLen)
      {
        const auto y = readPos - readPostionTrunc;
        
        for (auto chan=0; chan<NCHANS; chan++)
        {
          const auto xm1 = ((readPosInt - 1) > 0) ? inputs[chan][readPosInt - 1] : 0.0f;
          const auto x0 = ((readPosInt) < inputLen) ? inputs[chan][readPosInt] : inputs[chan][readPosInt-1];
          const auto x1 = ((readPosInt + 1) < inputLen) ? inputs[chan][readPosInt + 1] : inputs[chan][readPosInt-1];
          const auto x2 = ((readPosInt + 2) < inputLen) ? inputs[chan][readPosInt + 2] : inputs[chan][readPosInt-1];
          
          const auto  c = (x1 - xm1) * 0.5;
          const auto  v = x0 - x1;
          const auto  w = c + v;
          const auto  a = w + v + (x2 - x0) * 0.5;
          const auto  b = w + a;
          
          outputs[chan][writePos] = ((((a * y) -b) * y + c) * y + x0);
        }
      }
    }
    
    return outputLen;
  }
  
  WDL_TypedBuf<T> mResampledData;
  WDL_PtrList<T> mScratchPtrs;
  double mUpRatio = 0.0, mDownRatio = 0.0;
  double mInputSampleRate = 0.0;
  const double mRenderingSampleRate;
  ESRCMode mResamplingMode;
  
  std::unique_ptr<LanczosResampler<T, NCHANS>> mResamplerUp, mResamplerDown;
};

END_IPLUG_NAMESPACE
