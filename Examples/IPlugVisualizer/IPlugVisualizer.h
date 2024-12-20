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

class IPlugVisualizer final : public Plugin
{
public:
  IPlugVisualizer(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void OnReset() override;
  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  ISpectrumSender<2> mSender;
#endif
#if IPLUG_EDITOR
  void OnParentWindowResize(int width, int height) override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
#endif
};
