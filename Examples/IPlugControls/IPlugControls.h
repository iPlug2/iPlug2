#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kMode,
  kFreq1,
  kFreq2,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagDialogResult = 0,
  kCtrlTagVectorButton,
  kCtrlTagScope,
  kCtrlTagMeter,
  kCtrlTags
};

class IPlugControls : public IPlug
{
public:
  IPlugControls(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnIdle() override;
private:
  IVScopeControl<2>::IVScopeBallistics mScopeBallistics { kCtrlTagScope };
  IVMeterControl<2>::IVMeterBallistics mMeterBallistics { kCtrlTagMeter };
#endif
};
