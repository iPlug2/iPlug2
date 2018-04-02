#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugFaustGen.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

class IPlugEffect : public IPlug
{
public:
  IPlugEffect(IPlugInstanceInfo instanceInfo);

  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
  FAUST_BLOCK(Noise, mNoise, "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Noise.dsp", 1, 1);
  FAUST_BLOCK(Osc, mOsc1, "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.dsp", 1, 1);
  FAUST_BLOCK(Osc, mOsc2, "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.dsp", 1, 1);
};
