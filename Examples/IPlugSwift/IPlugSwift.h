#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "Smoothers.h"

using namespace iplug;

class IPlugSwift : public Plugin
{
public:
  IPlugSwift(const InstanceInfo& info);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  bool OnMessage(int messageTag, int controlTag, int dataSize, const void* pData) override;
  void OnParamChange(int paramIdx) override;
  
  FastSinOscillator<float> mOsc;
  float mFreqCPS = 440.f;
  LogParamSmooth<sample> mGainSmoother { 20.f };
};
