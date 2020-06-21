#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagInputMeter = 0,
  kCtrlTagOutputMeter,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugSurroundEffect final : public Plugin
{
public:
  IPlugSurroundEffect(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnIdle() override;
  
  IPeakSender<8> mInputPeakSender;
  IPeakSender<8> mOutputPeakSender;
#endif
};
