#pragma once

#include "IPlug_include_in_plug_hdr.h"

#if IPLUG_DSP
#include "Oscillator.h"
#include "ISender.h"
#endif

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum EControlTags
{
  kCtrlTagScope = 0,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugResponsiveUI final : public Plugin
{
public:
  IPlugResponsiveUI(const InstanceInfo& info);

#if IPLUG_EDITOR
  void OnParentWindowResize(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override { return true; }
#endif
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnIdle() override;
private:
  FastSinOscillator<sample> mOsc {440.f};
  IBufferSender<> mScopeSender;
#endif
};
