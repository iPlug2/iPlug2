#pragma once

#include "IPlug_include_in_plug_hdr.h"


#ifdef SAMPLE_TYPE_FLOAT
  #define WDL_FFT_REALSIZE 4
#else
  #define WDL_FFT_REALSIZE 8
#endif

#include "convoengine.h"

#if defined USE_WDL_RESAMPLER
  #include "resample.h"
#elif defined USE_R8BRAIN
  #include "CDSPResampler.h"
  using namespace r8b;
#endif

const int kNumPresets = 1;

enum EParams
{
  kParamDry = 0,
  kParamWet,
  kNumParams
};

using namespace iplug;

class IPlugConvoEngine final : public Plugin
{
public:
  IPlugConvoEngine(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
private:
  // Returns destination length
  inline int ResampleLength(int srcLength, double srcRate, double destRate) const
  {
    return int(destRate / srcRate * (double)srcLength + 0.5);
  }

  template <class I, class O> void Resample(const I* pSrc, int srcLength, double srcRate, O* pDst, int dstLength, double dstRate);
  
  static const float mIR[512];

  WDL_ImpulseBuffer mImpulse;
//  WDL_ConvolutionEngine_Div mEngine; // < low latency version
  WDL_ConvolutionEngine mEngine;
  
  static constexpr int mBlockLength = 64;

  #if defined USE_WDL_RESAMPLER
  WDL_Resampler mResampler;
  #elif defined USE_R8BRAIN
  std::unique_ptr<CDSPResampler16IR> mResampler;
  #endif

  double mSampleRate = 0.0;
#endif
};
