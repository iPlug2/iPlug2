#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugDrumSynth_DSP.h"

const int kNumPrograms = 1;

enum EParams
{
  kParamGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagMeter = 0,
  kCtrlTagKeyboard,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugDrumSynth : public Plugin
{
public:
  IPlugDrumSynth(const InstanceInfo& info);

#if IPLUG_EDITOR
  void OnMidiMsgUI(const IMidiMsg& msg) override;
#endif
  
#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  bool GetMidiNoteText(int noteNumber, char* text) const override;
private:
  DrumSynthDSP mDSP;
#endif
};
