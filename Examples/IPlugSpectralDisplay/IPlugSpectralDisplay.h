#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugSpectralDisplay final : public Plugin
{
public:
  IPlugSpectralDisplay(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  ISpectrumSender<> mSender;
#endif
};
