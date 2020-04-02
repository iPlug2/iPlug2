#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPrograms = 1;

enum EParams
{
  kParamGain = 0,
  kNumParams
};

enum EControlTags
{
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugMidiEffect final : public Plugin
{
public:
  IPlugMidiEffect(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
#endif
};
