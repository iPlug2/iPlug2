#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include <vector>

const int kNumPrograms = 1;

enum EParams
{
  kParamGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kNumCtrlTags
};

class IPlugMidiEffect : public IPlug
{
public:
  IPlugMidiEffect(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
private:
  std::vector<int> mHeldKeys;
#endif
};
