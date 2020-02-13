#pragma once

#include "IPlug_include_in_plug_hdr.h"
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

#if IPLUG_DSP
// will use EParams in IPlugInstrument_DSP.h
#include "IPlugInstrument_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagMeter = 0,
  kCtrlTagKeyboard,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugInstrument : public Plugin
{
public:
  IPlugInstrument(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
  bool OnKeyDown(const IKeyPress& key) override;
  bool OnKeyUp(const IKeyPress& key) override;

private:
  IPlugInstrumentDSP<sample> mDSP {16};
  IVMeterControl<1>::Sender mMeterSender {kCtrlTagMeter};
#endif
};
