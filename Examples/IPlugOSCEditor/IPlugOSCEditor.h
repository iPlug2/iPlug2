#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugOSC.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagGain = 0,
  kCtrlTagSendIP,
  kCtrlTagSendPort,
  kCtrlTagWebView,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugOSCEditor final : public Plugin, public OSCReceiver, public OSCSender
{
public:
  IPlugOSCEditor(const InstanceInfo& info);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
  void OnOSCMessage(OscMessageRead& msg) override;
};
