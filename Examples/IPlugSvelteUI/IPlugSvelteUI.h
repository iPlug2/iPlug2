#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "Smoothers.h"
#include "ISender.h"

using namespace iplug;

const int kNumPresets = 3;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum EMsgTags
{
  kMsgTagButton1 = 0,
  kMsgTagButton2 = 1,
  kMsgTagButton3 = 2,
  kMsgTagBinaryTest = 3
};

enum EControlTags
{
  kCtrlTagMeter = 0,
};

class IPlugSvelteUI final : public Plugin
{
public:
  IPlugSvelteUI(const InstanceInfo& info);
  
#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
#endif
  void OnIdle() override;

private:
  iplug::IPeakSender<2> mSender {-90., 20.f};
  FastSinOscillator<sample> mOscillator {0., 440.};
  LogParamSmooth<sample, 1> mGainSmoother;
};
