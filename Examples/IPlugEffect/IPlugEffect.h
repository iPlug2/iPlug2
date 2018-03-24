#pragma once

#include "IPlug_include_in_plug_hdr.h"

#include "IPlugFaustGen.h"

FAUST_BLOCK(Noise);
FAUST_BLOCK(Osc);

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
  
  Noise mFaustNoise;
  Osc mFaustOsc1;
  //, mFaustOsc2;
};
