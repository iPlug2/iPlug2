#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IGraphicsJSON.h"
#include <memory>

enum EParams
{
  kGain = 0,
  kPan,
  kMix,
  kNumParams
};

enum ECtrlTags
{
  kGainKnob = 0,
  kPanKnob,
  kMixSlider
};

using namespace iplug;
using namespace igraphics;

class IPlugJSONUI final : public Plugin
{
public:
  IPlugJSONUI(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

#if IPLUG_EDITOR
  void OnIdle() override;
#endif

private:
#if IPLUG_EDITOR
  std::unique_ptr<IGraphicsJSON> mJSONUI;
#endif
};
