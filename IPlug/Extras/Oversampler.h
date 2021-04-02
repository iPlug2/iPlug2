/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#define OVERSAMPLING_FACTORS_VA_LIST "None", "2x", "4x", "8x", "16x"

#include <functional>
#include <cmath>

#include "HIIR/FPUUpsampler2x.h"
#include "HIIR/FPUDownsampler2x.h"

#include "heapbuf.h"
#include "ptrlist.h"

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

using namespace hiir;

enum EFactor
{
  kNone = 0,
  k2x,
  k4x,
  k8x,
  k16x,
  kNumFactors
};

template<typename T = double>
class OverSampler
{
public:
  using BlockProcessFunc = std::function<void(T**, T**, int)>;
  
  OverSampler(EFactor factor = kNone, bool blockProcessing = true, int nInChannels = 1, int nOutChannels = 1)
  : mBlockProcessing(blockProcessing)
  , mNInChannels(nInChannels)
  , mNOutChannels(nOutChannels)
  {
    
    static constexpr double coeffs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };
    static constexpr double coeffs4x[4] = {0.041893991997656171, 0.16890348243995201, 0.39056077292116603, 0.74389574826847926 };
    static constexpr double coeffs8x[3] = {0.055748680811302048, 0.24305119574153072, 0.64669913119268196 };
    static constexpr double coeffs16x[2] = {0.10717745346023573, 0.53091435354504557 };

    for (auto c = 0; c < mNInChannels; c++)
    {
      mUpsampler2x.Add(new Upsampler2xFPU<12, T>());
      mUpsampler4x.Add(new Upsampler2xFPU<4, T>());
      mUpsampler8x.Add(new Upsampler2xFPU<3, T>());
      mUpsampler16x.Add(new Upsampler2xFPU<2, T>());
      
      mUpsampler2x.Get(c)->set_coefs(coeffs2x);
      mUpsampler4x.Get(c)->set_coefs(coeffs4x);
      mUpsampler8x.Get(c)->set_coefs(coeffs8x);
      mUpsampler16x.Get(c)->set_coefs(coeffs16x);
      
      // ptr location doesn't matter at this stage
      mNextInputPtrs.Add(mUp2x.Get());
    }
    
    for (auto c = 0; c < mNOutChannels; c++)
    {
      mDownsampler2x.Add(new Downsampler2xFPU<12, T>());
      mDownsampler4x.Add(new Downsampler2xFPU<4, T>());
      mDownsampler8x.Add(new Downsampler2xFPU<3, T>());
      mDownsampler16x.Add(new Downsampler2xFPU<2, T>());
      
      mDownsampler2x.Get(c)->set_coefs(coeffs2x);
      mDownsampler4x.Get(c)->set_coefs(coeffs4x);
      mDownsampler8x.Get(c)->set_coefs(coeffs8x);
      mDownsampler16x.Get(c)->set_coefs(coeffs16x);
      
      // ptr location doesn't matter at this stage
      mNextOutputPtrs.Add(mDown2x.Get());
    }
        
    SetOverSampling(factor);
    
    Reset();
  }
  
  ~OverSampler()
  {
    mUpsampler2x.Empty(true);
    mDownsampler2x.Empty(true);
    mUpsampler4x.Empty(true);
    mDownsampler4x.Empty(true);
    mUpsampler8x.Empty(true);
    mDownsampler8x.Empty(true);
    mUpsampler16x.Empty(true);
    mDownsampler16x.Empty(true);
  }

  OverSampler(const OverSampler&) = delete;
  OverSampler& operator=(const OverSampler&) = delete;
    
  void Reset(int blockSize = DEFAULT_BLOCK_SIZE)
  {
    int numBufSamples = 1;
    
    if(mBlockProcessing)
      numBufSamples = blockSize;
    else
    {
      numBufSamples = 1;
      blockSize = 1;
    }
    
    numBufSamples *= mNInChannels;
    
    mUp2x.Resize(2 * numBufSamples);
    mUp4x.Resize(4 * numBufSamples);
    mUp8x.Resize(8 * numBufSamples);
    mUp16x.Resize(16 * numBufSamples);
    
    mDown2x.Resize(2 * numBufSamples);
    mDown4x.Resize(4 * numBufSamples);
    mDown8x.Resize(8 * numBufSamples);
    mDown16x.Resize(16 * numBufSamples);
    
    mUp16BufferPtrs.Empty();
    mUp8BufferPtrs.Empty();
    mUp4BufferPtrs.Empty();
    mUp2BufferPtrs.Empty();
    
    mDown16BufferPtrs.Empty();
    mDown8BufferPtrs.Empty();
    mDown4BufferPtrs.Empty();
    mDown2BufferPtrs.Empty();
    
    for (auto c = 0; c < mNInChannels; c++)
    {
      mUpsampler2x.Get(c)->clear_buffers();
      mUpsampler4x.Get(c)->clear_buffers();
      mUpsampler8x.Get(c)->clear_buffers();
      mUpsampler16x.Get(c)->clear_buffers();
      
      mUp2BufferPtrs.Add(mUp2x.Get() + c * 2 * blockSize);
      mUp4BufferPtrs.Add(mUp4x.Get() + (c * 4 * blockSize));
      mUp8BufferPtrs.Add(mUp8x.Get() + (c * 8 * blockSize));
      mUp16BufferPtrs.Add(mUp16x.Get() + (c * 16 * blockSize));
    }
    
    for (auto c = 0; c < mNOutChannels; c++)
    {
      mDownsampler2x.Get(c)->clear_buffers();
      mDownsampler4x.Get(c)->clear_buffers();
      mDownsampler8x.Get(c)->clear_buffers();
      mDownsampler16x.Get(c)->clear_buffers();
      
      mDown2BufferPtrs.Add(mDown2x.Get() + c * 2 * blockSize);
      mDown4BufferPtrs.Add(mDown4x.Get() + (c * 4 * blockSize));
      mDown8BufferPtrs.Add(mDown8x.Get() + (c * 8 * blockSize));
      mDown16BufferPtrs.Add(mDown16x.Get() + (c * 16 * blockSize));
    }
  }

  /** Over sample an input block with a per-block function (up sample input -> process with function -> down sample)
   * @param inputs Two-dimensional array containing the non-interleaved input buffers of audio samples for all channels
   * @param outputs Two-dimensional array for audio output (non-interleaved).
   * @param nFrames The block size for this block: number of samples per channel.
   * @param nInChans The number of input channels to process. Must be less or equal to the number of channels passed to the constructor
   * @param nOutChans The number of output channels to process. Must be less or equal to the number of channels passed to the constructor
   * @param func The function that processes the audio sample at the higher sampling rate. NOTE: std::function can call malloc if you pass in captures */
  void ProcessBlock(T** inputs, T** outputs, int nFrames, int nInChans, int nOutChans, BlockProcessFunc func)
  {
    assert(nInChans <= mNInChannels);
    assert(nOutChans <= mNOutChannels);
    
    if(mRate != mPrevRate)
    {
      switch (mRate) {
        case 2:
          mInPtrLoopSrc = &mUp2BufferPtrs;
          mOutPtrLoopSrc = &mDown2BufferPtrs;
          break;
        case 4:
          mInPtrLoopSrc = &mUp4BufferPtrs;
          mOutPtrLoopSrc = &mDown4BufferPtrs;
          break;
        case 8:
          mInPtrLoopSrc = &mUp8BufferPtrs;
          mOutPtrLoopSrc = &mDown8BufferPtrs;
          break;
        case 16:
          mInPtrLoopSrc = &mUp16BufferPtrs;
          mOutPtrLoopSrc = &mDown16BufferPtrs;
          break;
        default:
          break;
      }
      
      mPrevRate = mRate;
    }

    for(auto c = 0; c < nInChans; c++) {
      if (mRate >= 2) {
        mUpsampler2x.Get(c)->process_block(mUp2BufferPtrs.Get(c), inputs[c], nFrames);
      }
      if (mRate >= 4) {
        mUpsampler4x.Get(c)->process_block(mUp4BufferPtrs.Get(c), mUp2BufferPtrs.Get(c), nFrames * 2);
      }
      if (mRate >= 8) {
        mUpsampler8x.Get(c)->process_block(mUp8BufferPtrs.Get(c), mUp4BufferPtrs.Get(c), nFrames * 4);
      }
      if (mRate == 16) {
        mUpsampler16x.Get(c)->process_block(mUp16BufferPtrs.Get(c), mUp8BufferPtrs.Get(c), nFrames * 8);
      }
    }
    
    if (mRate == 1) {
      func(inputs, outputs, nFrames);
    } else {
      for (auto i = 0; i < mRate; i++) {
        for(auto c = 0; c < nInChans; c++) {
          mNextInputPtrs.Set(c, mInPtrLoopSrc->Get(c) + (i * nFrames));
          mNextOutputPtrs.Set(c, mOutPtrLoopSrc->Get(c) + (i * nFrames));
        }
        func(mNextInputPtrs.GetList(), mNextOutputPtrs.GetList(), nFrames);
      }
    }
    
    for(auto c = 0; c < nOutChans; c++) {
      if (mRate == 16) {
        mDownsampler16x.Get(c)->process_block(mDown8BufferPtrs.Get(c), mDown16BufferPtrs.Get(c), nFrames * 8);
      }
      if (mRate >= 8) {
        mDownsampler8x.Get(c)->process_block(mDown4BufferPtrs.Get(c), mDown8BufferPtrs.Get(c), nFrames * 4);
      }
      if (mRate >= 4) {
        mDownsampler4x.Get(c)->process_block(mDown2BufferPtrs.Get(c), mDown4BufferPtrs.Get(c), nFrames * 2);
      }
      if (mRate >= 2) {
        mDownsampler2x.Get(c)->process_block(outputs[c], mDown2BufferPtrs.Get(c), nFrames);
      }
    }
  }
  
  /** Over sample an input sample with a per-sample function (up-sample input -> process with function -> down-sample)
   * @param input The audio sample to input
   * @param std::function<double(double)> The function that processes the audio sample at the higher sampling rate. NOTE: std::function can call malloc if you pass in captures
   * @return The audio sample output */
  T Process(T input, std::function<T(T)> func)
  {
    T output;

    if(mRate == 16)
    {
      mUpsampler2x.Get(0)->process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);
      mUpsampler4x.Get(0)->process_block(mUp4x.Get(), mUp2x.Get(), 2);
      mUpsampler8x.Get(0)->process_block(mUp8x.Get(), mUp4x.Get(), 4);
      mUpsampler16x.Get(0)->process_block(mUp16x.Get(), mUp8x.Get(), 8);

      for (auto i = 0; i < 16; i++)
      {
        mDown16x.Get()[i] = func(mUp16x.Get()[i]);
      }

      mDownsampler16x.Get(0)->process_block(mDown8x.Get(), mDown16x.Get(), 8);
      mDownsampler8x.Get(0)->process_block(mDown4x.Get(), mDown8x.Get(), 4);
      mDownsampler4x.Get(0)->process_block(mDown2x.Get(), mDown4x.Get(), 2);
      output = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
    }
    else if (mRate == 8)
    {
      mUpsampler2x.Get(0)->process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);
      mUpsampler4x.Get(0)->process_block(mUp4x.Get(), mUp2x.Get(), 2);
      mUpsampler8x.Get(0)->process_block(mUp8x.Get(), mUp4x.Get(), 4);

      for (auto i = 0; i < 8; i++)
      {
        mDown8x.Get()[i] = func(mUp8x.Get()[i]);
      }

      mDownsampler8x.Get(0)->process_block(mDown4x.Get(), mDown8x.Get(), 4);
      mDownsampler4x.Get(0)->process_block(mDown2x.Get(), mDown4x.Get(), 2);
      output = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
    }
    else if (mRate == 4)
    {
      mUpsampler2x.Get(0)->process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);
      mUpsampler4x.Get(0)->process_block(mUp4x.Get(), mUp2x.Get(), 2);

      for (auto i = 0; i < 4; i++)
      {
        mDown4x.Get()[i] = func(mUp4x.Get()[i]);
      }

      mDownsampler4x.Get(0)->process_block(mDown2x.Get(), mDown4x.Get(), 2);
      output = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
    }
    else if (mRate == 2)
    {
      mUpsampler2x.Get(0)->process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);

      mDown2x.Get()[0] = func(mUp2x.Get()[0]);
      mDown2x.Get()[1] = func(mUp2x.Get()[1]);
      output = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
    }
    else
    {
      output = func(input);
    }

    return output;
  }

  /** Over-sample an per-sample synthesis function
   * @param genFunc The function that generates the audio sample
   * @return The audio sample output */
  T ProcessGen(std::function<T()> genFunc)
  {
    auto ProcessDown16x = [&](T input)
    {
      mDown16x.Get()[mWritePos] = (T) input;

      mWritePos++;
      mWritePos &= 15;

      if(mWritePos == 0)
      {
        mDownsampler16x.Get(0)->process_block(mDown8x.Get(), mDown16x.Get(), 8);
        mDownsampler8x.Get(0)->process_block(mDown4x.Get(), mDown8x.Get(), 4);
        mDownsampler4x.Get(0)->process_block(mDown2x.Get(), mDown4x.Get(), 2);
        mDownSamplerOutput = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
      }
    };

    auto ProcessDown8x = [&](T input)
    {
      mDown8x.Get()[mWritePos] = (T) input;

      mWritePos++;
      mWritePos &= 7;

      if(mWritePos == 0)
      {
        mDownsampler8x.Get(0)->process_block(mDown4x.Get(), mDown8x.Get(), 4);
        mDownsampler4x.Get(0)->process_block(mDown2x.Get(), mDown4x.Get(), 2);
        mDownSamplerOutput = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
      }
    };

    auto ProcessDown4x = [&](T input)
    {
      mDown4x.Get()[mWritePos] = (T) input;

      mWritePos++;
      mWritePos &= 3;

      if(mWritePos == 0)
      {
        mDownsampler4x.Get(0)->process_block(mDown2x.Get(), mDown4x.Get(), 2);
        mDownSamplerOutput = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
      }
    };

    auto ProcessDown2x = [&](T input)
    {
      mDown2x.Get()[mWritePos] = (T) input;

      mWritePos = !mWritePos;

      if(mWritePos == 0)
      {
        mDownSamplerOutput = mDownsampler2x.Get(0)->process_sample(mDown2x.Get());
      }
    };

    T output;

    for (int j = 0; j < mRate; j++)
    {
      output = genFunc();

      switch(mRate)
      {
        case 2: ProcessDown2x(output); break;
        case 4: ProcessDown4x(output); break;
        case 8: ProcessDown8x(output); break;
        case 16: ProcessDown16x(output); break;
        default: break;
      }
    }

    if(mRate > 1)
      output = mDownSamplerOutput;

    return output;
  }

  void SetOverSampling(EFactor factor)
  {
    if(factor != mFactor)
    {
      mFactor = factor;
      mRate = std::pow(2, (int) factor);
      
      Reset();
    }
  }
  
  static EFactor RateToFactor(int rate)
  {
    switch (rate)
    {
      case 1: return EFactor::kNone;
      case 2: return EFactor::k2x;
      case 4: return EFactor::k4x;
      case 8: return EFactor::k8x;
      case 16: return EFactor::k16x;
      default: assert(0); return EFactor::kNone;
    }
  }
  
  int GetRate()
  {
    return mRate;
  }

private:
  EFactor mFactor = kNone;
  int mPrevRate = 0;
  int mRate = 1;
  int mWritePos = 0;
  T mDownSamplerOutput = 0.;
  bool mBlockProcessing; // false
  int mNInChannels; // 1
  int mNOutChannels;
  
  // the actual data
  WDL_TypedBuf<T> mUp16x;
  WDL_TypedBuf<T> mUp8x;
  WDL_TypedBuf<T> mUp4x;
  WDL_TypedBuf<T> mUp2x;

  WDL_TypedBuf<T> mDown16x;
  WDL_TypedBuf<T> mDown8x;
  WDL_TypedBuf<T> mDown4x;
  WDL_TypedBuf<T> mDown2x;
  
  //Ptrs into buffer data
  WDL_PtrList<T> mUp16BufferPtrs;
  WDL_PtrList<T> mUp8BufferPtrs;
  WDL_PtrList<T> mUp4BufferPtrs;
  WDL_PtrList<T> mUp2BufferPtrs;
  
  WDL_PtrList<T> mDown16BufferPtrs;
  WDL_PtrList<T> mDown8BufferPtrs;
  WDL_PtrList<T> mDown4BufferPtrs;
  WDL_PtrList<T> mDown2BufferPtrs;

  WDL_PtrList<T> mNextInputPtrs;
  WDL_PtrList<T> mNextOutputPtrs;

  //Ptrs to the buffer data ptrs, changed depending on rate (block processing only)
  WDL_PtrList<T>* mInPtrLoopSrc = nullptr;
  WDL_PtrList<T>* mOutPtrLoopSrc = nullptr;
  
  //Ptrs to oversamplers for each channel
  WDL_PtrList<Upsampler2xFPU<12, T>> mUpsampler2x; // for 1x to 2x SR
  WDL_PtrList<Upsampler2xFPU<4, T>> mUpsampler4x;  // for 2x to 4x SR
  WDL_PtrList<Upsampler2xFPU<3, T>> mUpsampler8x;  // for 4x to 8x SR
  WDL_PtrList<Upsampler2xFPU<2, T>> mUpsampler16x; // for 8x to 16x SR

  WDL_PtrList<Downsampler2xFPU<12, T>> mDownsampler2x; // decimator for 2x to 1x SR
  WDL_PtrList<Downsampler2xFPU<4, T>> mDownsampler4x;  // decimator for 4x to 2x SR
  WDL_PtrList<Downsampler2xFPU<3, T>> mDownsampler8x;  // decimator for 8x to 4x SR
  WDL_PtrList<Downsampler2xFPU<2, T>> mDownsampler16x; // decimator for 16x to 8x SR
};

END_IPLUG_NAMESPACE
