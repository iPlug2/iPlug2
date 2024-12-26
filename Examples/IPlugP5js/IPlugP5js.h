#pragma once

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

class IPlugP5js final : public Plugin
{
public:
  IPlugP5js(const InstanceInfo& info);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
};
