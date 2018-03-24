#pragma once

#include "IPlug_include_in_plug_hdr.h"

#ifndef FAUST_COMPILED
  #include "IPlugFaustGen.h"
  #define FGain FaustGen
  #define FOsc FaustGen
#else
  #include "Gain.hpp"
  #include "Osc.hpp"
#endif

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
  
  FGain mFaustGain;
  FOsc mFaustOsc;
};
