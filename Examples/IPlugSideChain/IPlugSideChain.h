#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#include "IControls.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagInputMeter = 0,
  kCtrlTagOutputMeter,
  kNumCtrlTags
};

enum EMsgTags
{
  kMsgTagConnectionsChanged = 0,
  kNumMsgTags
};

using namespace iplug;
using namespace igraphics;

class IPlugSideChain final : public Plugin
{
public:
  IPlugSideChain(const InstanceInfo& info);

  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnActivate(bool enable) override;
  void OnReset() override;
  void GetBusName(ERoute direction, int busIdx, int nBuses, WDL_String& str) const override;

  bool mInputChansConnected[4] = {};
  bool mOutputChansConnected[2] = {};
  bool mSendUpdate = false;
  
  IPeakAvgSender<4> mInputPeakSender;
  IPeakAvgSender<2> mOutputPeakSender;
  IVMeterControl<4>* mInputMeter = nullptr;
  IVMeterControl<2>* mOutputMeter = nullptr;
};
