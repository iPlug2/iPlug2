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
//#include "HIIR/PolyphaseIIR2Designer.h"

#include "heapbuf.h"
#include "ptrlist.h"

using namespace hiir;

template<typename T = double>
class OverSampler
{
public:

  typedef std::function<void(T**, T**, int)> BlockProcessFunc;
  
  enum EFactor
  {
    kNone = 0,
    k2x,
    k4x,
    k8x,
    k16x,
    kNumFactors
  };

  OverSampler(EFactor factor = kNone, bool blockProcessing = false, int nChannels = 1)
  : mBlockProcessing(blockProcessing)
  , mNChannels(nChannels)
  {
    SetOverSampling(factor);
    
    static double coeffs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };
;
//    PolyphaseIir2Designer::compute_coefs(coeffs2x, 96., 0.01);

//    printf("coeffs2x\n");
//
//    for(int i=0;i<12;i++)
//      printf("%.17g,\n", coeffs2x[i]);

    mUpsampler2x.set_coefs(coeffs2x);
    mDownsampler2x.set_coefs(coeffs2x);

    static double coeffs4x[4] = {0.041893991997656171, 0.16890348243995201, 0.39056077292116603, 0.74389574826847926 };

//    PolyphaseIir2Designer::compute_coefs(coeffs4x, 96., 0.255);

//    printf("coeffs4x\n");
//
//    for(int i=0;i<4;i++)
//      printf("%.17g,\n", coeffs4x[i]);

    mUpsampler4x.set_coefs(coeffs4x);
    mDownsampler4x.set_coefs(coeffs4x);

    static double coeffs8x[3] = {0.055748680811302048, 0.24305119574153072, 0.64669913119268196 };

//  PolyphaseIir2Designer::compute_coefs(coeffs8x, 96., 0.3775);

//    printf("coeffs8x\n");
//
//    for(int i=0;i<3;i++)
//      printf("%.17g,\n", coeffs8x[i]);

    mUpsampler8x.set_coefs(coeffs8x);
    mDownsampler8x.set_coefs(coeffs8x);

    static double coeffs16x[2] = {0.10717745346023573, 0.53091435354504557 };

//    PolyphaseIir2Designer::compute_coefs(coeffs16x, 96., 0.43865);

//    printf("coeffs16x\n");
//
//    for(int i=0;i<2;i++)
//      printf("%.17g,\n", coeffs16x[i]);

    mUpsampler16x.set_coefs(coeffs16x);
    mDownsampler16x.set_coefs(coeffs16x);

    Reset();
  }

  void Reset(int blockSize = DEFAULT_BLOCK_SIZE)
  {
    //TODO: methinks perhaps a for loop?
    
    mUpsampler2x.clear_buffers();
    mUpsampler4x.clear_buffers();
    mUpsampler8x.clear_buffers();
    mUpsampler16x.clear_buffers();
    mDownsampler2x.clear_buffers();
    mDownsampler4x.clear_buffers();
    mDownsampler8x.clear_buffers();
    mDownsampler16x.clear_buffers();
    
    int numBufSamples = 1;
    
    if(mBlockProcessing)
      numBufSamples = blockSize;
    else
      blockSize = 1;
    
    numBufSamples *= mNChannels;
    
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
    
    for (auto c = 0; c < mNChannels; c++) // TODO: doesn't work
    {
      mUp2BufferPtrs.Add(mUp2x.Get() + (c * 2 * blockSize));
      mUp4BufferPtrs.Add(mUp4x.Get() + (c * 4 * blockSize));
      mUp8BufferPtrs.Add(mUp8x.Get() + (c * 8 * blockSize));
      mUp16BufferPtrs.Add(mUp16x.Get() + (c * 16 * blockSize));
      mDown2BufferPtrs.Add(mDown2x.Get() + (c * 2 * blockSize));
      mDown4BufferPtrs.Add(mDown4x.Get() + (c * 4 * blockSize));
      mDown8BufferPtrs.Add(mDown8x.Get() + (c * 8 * blockSize));
      mDown16BufferPtrs.Add(mDown16x.Get() + (c * 16 * blockSize));
    }
  }

//  void ProcessBlock(T** inputs, T** outputs, int nFrames, int nChans, BlockProcessFunc func)
//  {
//    assert(nChans <= mNChannels);
//
//    if (mRate == 2)
//    {
//      // for each channel upsample block
//      for(auto c = 0; c < nChans; c++)
//      {
//        mUpsampler2x.process_block(mUp2BufferPtrs.Get(c), inputs[c], nFrames);
//      }
//
//      func(mUp2BufferPtrs.GetList(), mDown2BufferPtrs.GetList(), nFrames);
//
//      // TODO: move pointers in a better way! TODO: this doesn't actually work
//      WDL_PtrList<T> nextInputPtrs;
//      WDL_PtrList<T> nextOutputPtrs;
//
//      for(auto c = 0; c < nChans; c++)
//      {
//        nextInputPtrs.Add(mUp2BufferPtrs.Get(c) + nFrames);
//        nextOutputPtrs.Add(mDown2BufferPtrs.Get(c) + nFrames);
//      }
//
//      func(nextInputPtrs.GetList(), nextOutputPtrs.GetList(), nFrames);
//
//      for(auto c = 0; c < nChans; c++)
//      {
//        mDownsampler2x.process_block(outputs[c], mDown2BufferPtrs.Get(c), nFrames);
//      }
//    }
//    else
//    {
//      func(inputs, outputs, nFrames);
//    }
//  }
  
  /** Over sample an input sample with a per-sample function (up sample input -> process with function -> down sample)
   * @param input The audio sample to input
   * @param std::function<double(double)> The function that processes the audio sample at the higher sampling rate
   * @param mRate A power of 2 oversampling factor or 0 for no oversampling
   * @return The audio sample output */
  T Process(T input, std::function<T(T)> func)
  {
    T output;

    if(mRate == 16)
    {
      mUpsampler2x.process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);
      mUpsampler4x.process_block(mUp4x.Get(), mUp2x.Get(), 2);
      mUpsampler8x.process_block(mUp8x.Get(), mUp4x.Get(), 4);
      mUpsampler16x.process_block(mUp16x.Get(), mUp8x.Get(), 8);

      for (auto i = 0; i < 16; i++)
      {
        mDown16x.Get()[i] = func(mUp16x.Get()[i]);
      }

      mDownsampler16x.process_block(mDown8x.Get(), mDown16x.Get(), 8);
      mDownsampler8x.process_block(mDown4x.Get(), mDown8x.Get(), 4);
      mDownsampler4x.process_block(mDown2x.Get(), mDown4x.Get(), 2);
      output = mDownsampler2x.process_sample(mDown2x.Get());
    }
    else if (mRate == 8)
    {
      mUpsampler2x.process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);
      mUpsampler4x.process_block(mUp4x.Get(), mUp2x.Get(), 2);
      mUpsampler8x.process_block(mUp8x.Get(), mUp4x.Get(), 4);

      for (auto i = 0; i < 8; i++)
      {
        mDown8x.Get()[i] = func(mUp8x.Get()[i]);
      }

      mDownsampler8x.process_block(mDown4x.Get(), mDown8x.Get(), 4);
      mDownsampler4x.process_block(mDown2x.Get(), mDown4x.Get(), 2);
      output = mDownsampler2x.process_sample(mDown2x.Get());
    }
    else if (mRate == 4)
    {
      mUpsampler2x.process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);
      mUpsampler4x.process_block(mUp4x.Get(), mUp2x.Get(), 2);

      for (auto i = 0; i < 4; i++)
      {
        mDown4x.Get()[i] = func(mUp4x.Get()[i]);
      }

      mDownsampler4x.process_block(mDown2x.Get(), mDown4x.Get(), 2);
      output = mDownsampler2x.process_sample(mDown2x.Get());
    }
    else if (mRate == 2)
    {
      mUpsampler2x.process_sample(mUp2x.Get()[0], mUp2x.Get()[1], input);

      mDown2x.Get()[0] = func(mUp2x.Get()[0]);
      mDown2x.Get()[1] = func(mUp2x.Get()[1]);
      output = mDownsampler2x.process_sample(mDown2x.Get());
    }
    else
    {
      output = func(input);
    }

    return output;
  }

  T ProcessGen(std::function<T()> genFunc)
  {
    auto ProcessDown16x = [&](T input)
    {
      mDown16x[mWritePos] = (T) input;

      mWritePos++;
      mWritePos &= 15;

      if(mWritePos == 0)
      {
        mDownsampler16x.process_block(mDown8x, mDown16x, 8);
        mDownsampler8x.process_block(mDown4x, mDown8x, 4);
        mDownsampler4x.process_block(mDown2x, mDown4x, 2);
        mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
      }
    };

    auto ProcessDown8x = [&](T input)
    {
      mDown8x[mWritePos] = (T) input;

      mWritePos++;
      mWritePos &= 7;

      if(mWritePos == 0)
      {
        mDownsampler8x.process_block(mDown4x, mDown8x, 4);
        mDownsampler4x.process_block(mDown2x, mDown4x, 2);
        mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
      }
    };

    auto ProcessDown4x = [&](T input)
    {
      mDown4x[mWritePos] = (T) input;

      mWritePos++;
      mWritePos &= 3;

      if(mWritePos == 0)
      {
        mDownsampler4x.process_block(mDown2x, mDown4x, 2);
        mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
      }
    };

    auto ProcessDown2x = [&](T input)
    {
      mDown2x[mWritePos] = (T) input;

      mWritePos = !mWritePos;

      if(mWritePos == 0)
      {
        mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
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
    mRate = std::pow(2, (int) factor);
    Reset();
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
  int mRate = 1;
  int mWritePos = 0;
  T mDownSamplerOutput = 0.;
  bool mBlockProcessing; // false
  int mNChannels; // 1
  
  WDL_TypedBuf<T> mUp16x;
  WDL_TypedBuf<T> mUp8x;
  WDL_TypedBuf<T> mUp4x;
  WDL_TypedBuf<T> mUp2x;

  WDL_TypedBuf<T> mDown16x;
  WDL_TypedBuf<T> mDown8x;
  WDL_TypedBuf<T> mDown4x;
  WDL_TypedBuf<T> mDown2x;
  
  WDL_PtrList<T> mUp16BufferPtrs;
  WDL_PtrList<T> mUp8BufferPtrs;
  WDL_PtrList<T> mUp4BufferPtrs;
  WDL_PtrList<T> mUp2BufferPtrs;
  
  WDL_PtrList<T> mDown16BufferPtrs;
  WDL_PtrList<T> mDown8BufferPtrs;
  WDL_PtrList<T> mDown4BufferPtrs;
  WDL_PtrList<T> mDown2BufferPtrs;

  Upsampler2xFPU<12, T> mUpsampler2x; // for 1x to 2x SR
  //TODO: these could be replaced by cheaper alternatives
  Upsampler2xFPU<4, T> mUpsampler4x;  // for 2x to 4x SR
  Upsampler2xFPU<3, T> mUpsampler8x;  // for 4x to 8x SR
  Upsampler2xFPU<2, T> mUpsampler16x; // for 8x to 16x SR

  Downsampler2xFPU<12, T> mDownsampler2x; // decimator for 2x to 1x SR
  //TODO: these could be replaced by cheaper alternatives
  Downsampler2xFPU<4, T> mDownsampler4x;  // decimator for 4x to 2x SR
  Downsampler2xFPU<3, T> mDownsampler8x;  // decimator for 8x to 4x SR
  Downsampler2xFPU<2, T> mDownsampler16x; // decimator for 16x to 8x SR
};
