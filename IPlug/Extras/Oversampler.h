#pragma once

#define OVERSAMPLING_FACTORS_VA_LIST "None", "2x", "4x", "8x", "16x"

#include <functional>

#include "HIIR/FPUUpsampler2x.h"
#include "HIIR/FPUDownsampler2x.h"
//#include "HIIR/PolyphaseIIR2Designer.h"

using namespace hiir;

template<typename T = double>
class OverSampler
{
public:

  enum EFactor
  {
    kNone = 0,
    k2x,
    k4x,
    k8x,
    k16x,
    kNumFactors
  };

  OverSampler(EFactor factor = kNone)
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

  void Reset()
  {
    mUpsampler2x.clear_buffers();
    mUpsampler4x.clear_buffers();
    mUpsampler8x.clear_buffers();
    mUpsampler16x.clear_buffers();
    mDownsampler2x.clear_buffers();
    mDownsampler4x.clear_buffers();
    mDownsampler8x.clear_buffers();
    mDownsampler16x.clear_buffers();
  }

  /** Over sample an input sample with a per-sample function (up sample input -> process with function -> down sample)
   * @param input The audio sample to input
   * @param std::function<double(double)> The function that processes the audio sample at the higher sampling rate
   * @param mRate A power of 2 oversampling factor or 0 for no oversampling
   * @return The audio sample output */
  double Process(double input, std::function<T(T)> func)
  {
    double output;

    if(mRate == 16)
    {
      mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);
      mUpsampler4x.process_block(mUp4x, mUp2x, 2);
      mUpsampler8x.process_block(mUp8x, mUp4x, 4);
      mUpsampler16x.process_block(mUp16x, mUp8x, 8);

      for (auto i = 0; i < 16; i++)
      {
        mDown16x[i] = func(mUp16x[i]);
      }

      mDownsampler16x.process_block(mDown8x, mDown16x, 8);
      mDownsampler8x.process_block(mDown4x, mDown8x, 4);
      mDownsampler4x.process_block(mDown2x, mDown4x, 2);
      output = mDownsampler2x.process_sample(mDown2x);
    }
    else if (mRate == 8)
    {
      mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);
      mUpsampler4x.process_block(mUp4x, mUp2x, 2);
      mUpsampler8x.process_block(mUp8x, mUp4x, 4);

      for (auto i = 0; i < 8; i++)
      {
        mDown8x[i] = func(mUp8x[i]);
      }

      mDownsampler8x.process_block(mDown4x, mDown8x, 4);
      mDownsampler4x.process_block(mDown2x, mDown4x, 2);
      output = mDownsampler2x.process_sample(mDown2x);
    }
    else if (mRate == 4)
    {
      mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);
      mUpsampler4x.process_block(mUp4x, mUp2x, 2);

      for (auto i = 0; i < 4; i++)
      {
        mDown4x[i] = func(mUp4x[i]);
      }

      mDownsampler4x.process_block(mDown2x, mDown4x, 2);
      output = mDownsampler2x.process_sample(mDown2x);
    }
    else if (mRate == 2)
    {
      mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);

      mDown2x[0] = func(mUp2x[0]);
      mDown2x[1] = func(mUp2x[1]);
      output = mDownsampler2x.process_sample(mDown2x);
    }
    else
    {
      output = func(input);
    }

    return output;
  }

  double ProcessGen(std::function<T()> genFunc)
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

    auto ProcessDown8x = [&](double input)
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

    auto ProcessDown4x = [&](double input)
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

    auto ProcessDown2x = [&](double input)
    {
      mDown2x[mWritePos] = (T) input;

      mWritePos = !mWritePos;

      if(mWritePos == 0)
      {
        mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
      }
    };

    double output;

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
    mRate = pow(2, (int) factor);
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

  //TODO: is it necessary/lower to have all these different arrays?
  T mUp16x[16] = {};
  T mUp8x[8] = {};
  T mUp4x[4] = {};
  T mUp2x[2] = {};

  T mDown16x[16] = {};
  T mDown8x[8] = {};
  T mDown4x[4] = {};
  T mDown2x[2] = {};

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
