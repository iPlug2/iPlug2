#pragma once

#include <functional>

#include "HIIR/FPUUpsampler2x.h"
#include "HIIR/FPUDownsampler2x.h"
//#include "HIIR/PolyphaseIIR2Designer.h"

using namespace hiir;

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
  
  OverSampler()
  : mWritePos(0)
  , mDownSamplerOutput(0.f)
  {
    memset(mUp16x, 0, 16 * sizeof(float));
    memset(mUp8x, 0, 8 * sizeof(float));
    memset(mUp4x, 0, 4 * sizeof(float));
    memset(mUp2x, 0, 2 * sizeof(float));

    memset(mDown16x, 0, 16 * sizeof(float));
    memset(mDown8x, 0, 8 * sizeof(float));
    memset(mDown4x, 0, 4 * sizeof(float));
    memset(mDown2x, 0, 2 * sizeof(float));

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

    ClearBuffers();
  }

  ~OverSampler()
  {
  }

  inline void ProcessUp16x(double input)
  {
    mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);
    mUpsampler4x.process_block(mUp2x, mUp4x, 2);
    mUpsampler8x.process_block(mUp4x, mUp8x, 4);
    mUpsampler16x.process_block(mUp8x, mUp16x, 8);
  }

  inline void ProcessUp8x(double input)
  {
    mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);
    mUpsampler4x.process_block(mUp2x, mUp4x, 2);
    mUpsampler8x.process_block(mUp4x, mUp8x, 4);
  }

  inline void ProcessUp4x(double input)
  {
    mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);
    mUpsampler4x.process_block(mUp4x, mUp2x, 2);
  }

  inline void ProcessUp2x(double input)
  {
    mUpsampler2x.process_sample(mUp2x[0], mUp2x[1], input);
  }

  inline void ProcessDown16x(double input)
  {
    mDown16x[mWritePos] = (float) input;

    mWritePos++;
    mWritePos &= 15;

    if(mWritePos == 0)
    {
      mDownsampler16x.process_block(mDown8x, mDown16x, 8);
      mDownsampler8x.process_block(mDown4x, mDown8x, 4);
      mDownsampler4x.process_block(mDown2x, mDown4x, 2);
      mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
    }
  }

  inline void ProcessDown8x(double input)
  {
    mDown8x[mWritePos] = (float) input;

    mWritePos++;
    mWritePos &= 7;

    if(mWritePos == 0)
    {
      mDownsampler8x.process_block(mDown4x, mDown8x, 4);
      mDownsampler4x.process_block(mDown2x, mDown4x, 2);
      mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
    }
  }

  inline void ProcessDown4x(double input)
  {
    mDown4x[mWritePos] = (float) input;

    mWritePos++;
    mWritePos &= 3;

    if(mWritePos == 0)
    {
      mDownsampler4x.process_block(mDown2x, mDown4x, 2);
      mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
    }
  }

  inline void ProcessDown2x(double input)
  {
    mDown2x[mWritePos] = (float) input;

    mWritePos = !mWritePos;

    if(mWritePos == 0)
    {
      mDownSamplerOutput = mDownsampler2x.process_sample(mDown2x);
    }
  }

  double GetDownSamplerOutput()
  {
    return (double) mDownSamplerOutput;
  }

  void ClearBuffers()
  {
    mDownsampler2x.clear_buffers();
    mDownsampler4x.clear_buffers();
    mDownsampler8x.clear_buffers();
    mDownsampler16x.clear_buffers();
  }

  /**
   Over sample an input function (up sample input -> process with function -> down sample)

   @param input The audio sample to input
   @param std::function<double(double)> The function that processes the audio sample at the higher sampling rate
   @param mOverSamplingFactor A power of 2 oversampling factor or 0 for no oversampling
   @return The audio sample output
   */
  double Process(double input, std::function<double(double)> func)
  {
    double upSampledInput, output;

    upSampledInput = input;

    for (int j = 0; j < mOverSamplingFactor; j++)
    {
      switch(mOverSamplingFactor) // we are switching on mOverSamplingFactor, NOT j so calls to one of the functions will happen mOverSamplingFactor times
      {
        case 2: ProcessUp2x(upSampledInput); break;
        case 4: ProcessUp4x(upSampledInput); break;
        case 8: ProcessUp8x(upSampledInput); break;
        case 16: ProcessUp16x(upSampledInput); break;
        default: break;
      }
    }

    for (int j = 0; j < mOverSamplingFactor; j++)
    {
      output = func(upSampledInput); // func gets executed mOverSamplingFactor times

      switch(mOverSamplingFactor)
      {
        case 2: ProcessDown2x(output); break;
        case 4: ProcessDown4x(output); break;
        case 8: ProcessDown8x(output); break;
        case 16: ProcessDown16x(output); break;
        default: break;
      }
    }

    if(mOverSamplingFactor > 1)
      output = GetDownSamplerOutput();

    return output;
  }

  double ProcessGen(std::function<double()> genFunc)
  {
    double output;

    for (int j = 0; j < mOverSamplingFactor; j++)
    {
      output = genFunc();

      switch(mOverSamplingFactor)
      {
        case 2: ProcessDown2x(output); break;
        case 4: ProcessDown4x(output); break;
        case 8: ProcessDown8x(output); break;
        case 16: ProcessDown16x(output); break;
        default: break;
      }
    }

    if(mOverSamplingFactor > 1)
      output = GetDownSamplerOutput();

    return output;
  }
  
  void SetOverSampling(EFactor factor)
  {
    mOverSamplingFactor = pow(2, (int) factor);
  }

private:
  int mOverSamplingFactor = 1;
  int mWritePos;
  float mDownSamplerOutput = 0.f;

  float mUp16x[16];
  float mUp8x[8];
  float mUp4x[4];
  float mUp2x[2];

  float mDown16x[16];
  float mDown8x[8];
  float mDown4x[4];
  float mDown2x[2];

  Upsampler2xFPU<2> mUpsampler2x; // for 1x to 2x SR
  Upsampler2xFPU<4> mUpsampler4x;  // for 2x to 4x SR
  Upsampler2xFPU<3> mUpsampler8x;  // for 4x to 8x SR
  Upsampler2xFPU<2> mUpsampler16x; // for 8x to 16x SR
  Downsampler2xFPU<2> mDownsampler2x; // decimator for 2x to 1x SR
  Downsampler2xFPU<4> mDownsampler4x;  // decimator for 4x to 2x SR
  Downsampler2xFPU<3> mDownsampler8x;  // decimator for 8x to 4x SR
  Downsampler2xFPU<2> mDownsampler16x; // decimator for 16x to 8x SR
};
