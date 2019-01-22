#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugInstrument_DSP.h"
#include "IVMeterControl.h"

const int kNumPrograms = 1;

enum EParams
{
  kParamGain = 0,
  kParamNoteGlideTime,
  kParamAttack,
  kParamDecay,
  kParamSustain,
  kParamRelease,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagMeter = 0,
  kNumCtrlTags
};

class IPlugInstrument : public IPlug
{
public:
  IPlugInstrument(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
private:
  IPlugInstrumentDSP mDSP {16};
  IVMeterControl<1>::IVMeterBallistics mMeterBallistics {kCtrlTagMeter};
#endif
};
