#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugDrumSynth_DSP.h"
#include "ISender.h"

const int kNumPresets = 1;

enum EParams
{
  kParamGain = 0,
  kParamMultiOuts,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagPad1 = 0,
  kCtrlTagPad2,
  kCtrlTagPad3,
  kCtrlTagPad4,
  kCtrlTagMeter,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugDrumSynth : public Plugin
{
public:
  IPlugDrumSynth(const InstanceInfo& info);

#if IPLUG_EDITOR // // http://bit.ly/2S64BDd
  void OnMidiMsgUI(const IMidiMsg& msg) override;
#endif
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
  void GetBusName(ERoute direction, int busIdx, int nBuses, WDL_String& str) const override;

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  bool GetMidiNoteText(int noteNumber, char* text) const override;
  void OnIdle() override;
private:
  DrumSynthDSP mDSP;
  IPeakSender<8> mSender;
#endif
};
