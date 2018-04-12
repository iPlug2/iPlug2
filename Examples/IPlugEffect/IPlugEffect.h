#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Extras/Oscillator.h"
const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ELayout
{
  kTextX = 10,
  kTextY = 10,
  kGainX = 100,
  kGainY = 100
};

class IPlugEffect : public IPlug
{
public:
  IPlugEffect(IPlugInstanceInfo instanceInfo);
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
private:
  FastSinOscillator<sample> mOsc {0., 440.};
};
