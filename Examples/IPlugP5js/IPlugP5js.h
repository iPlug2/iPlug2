#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"

using namespace iplug;

const int kNumPresets = 1;

// NB: resources/web/sketch.js mirrors kOctave's index (kParamOctave) and the
// octave count - keep them in sync if this enum changes.
enum EParams
{
  kGain = 0,
  kOctave,
  kNumParams
};

class IPlugP5js final : public Plugin
{
public:
  IPlugP5js(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
#endif

private:
  FastSinOscillator<sample> mOscillator {0., 110.};
};
