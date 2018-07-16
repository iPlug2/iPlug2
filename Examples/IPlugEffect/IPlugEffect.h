#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "IVMeterControl.h"
#include "IVScopeControl.h"
#include "IPlugEffect_DSP.h"

const int kNumPrograms = 1;

#define MAX_VOICES 32

enum EControlTags
{
  kControlTagMeter = 0,
  kControlTagScope,
  kNumControlTags
};

enum EParams
{
  kGain = 0,
  kNumParams
};

class PLUG_CLASS_NAME : public IPlug
{
public:
  PLUG_CLASS_NAME(IPlugInstanceInfo instanceInfo);
  
#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  void CreateUI() override;
#endif

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  
  FastSinOscillator<sample> mOsc {0., 440.};
  IVMeterControl<2>::IVMeterBallistics mMeterBallistics { kControlTagMeter };
  IVScopeControl<1>::IVScopeBallistics mScopeBallistics { kControlTagScope };
  IPlugEffectDSP mDSP {16};
  
  void OnIdle() override;
#endif
};
