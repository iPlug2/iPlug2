#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "RealtimeResampler.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugSRC final : public Plugin
{
public:
  IPlugSRC(const InstanceInfo& info);

  void OnReset() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  RealtimeResampler<sample> mRealtimeResampler;
};
