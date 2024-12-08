#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

const int kNumPresets = 1;

enum EParams
{
  kOctaveGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugChromogram final : public Plugin
{
public:
  IPlugChromogram(const InstanceInfo& info);

  void OnReset() override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  ISpectrumSender<2> mSender;
#endif
  void OnParentWindowResize(int width, int height) override;

  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
};
