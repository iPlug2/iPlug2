#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagPlatformView = 0,
  kCtrlTagTabs,
  kCtrlTagInputMeters,
  kCtrlTagOutputMeters,
};

using namespace iplug;
using namespace igraphics;

class IPlugEmbedPlatformView final : public Plugin
{
public:
  IPlugEmbedPlatformView(const InstanceInfo& info);
//  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
};
