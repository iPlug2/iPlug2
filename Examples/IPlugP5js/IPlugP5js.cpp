#include "IPlugP5js.h"
#include "IPlug_include_in_plug_src.h"

namespace
{
constexpr double kOctaveFrequencies[] = {110., 220., 440., 880., 1760.};
constexpr int kNumOctaves = static_cast<int>(sizeof(kOctaveFrequencies) / sizeof(kOctaveFrequencies[0]));
}

IPlugP5js::IPlugP5js(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", -12., -70, 0.);
  GetParam(kOctave)->InitEnum("Octave", 0, {"110 Hz", "220 Hz", "440 Hz", "880 Hz", "1760 Hz"});

#ifdef WEBVIEW_EDITOR_DELEGATE
  SetCustomUrlScheme("iplug2");
  SetEnableDevTools(true);
  
  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    EnableScroll(false);
  };
#endif
  
  MakeDefaultPreset();
}

#if IPLUG_DSP
void IPlugP5js::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  (void) inputs;

  const auto gain = static_cast<sample>(GetParam(kGain)->DBToAmp());
  // kOctave is an InitEnum param, so Int() is already bounded to [0, kNumOctaves-1].
  const int octave = GetParam(kOctave)->Int();
  const int nChans = NOutChansConnected();

  mOscillator.SetFreqCPS(kOctaveFrequencies[octave]);

  for (int s = 0; s < nFrames; s++)
  {
    const sample output = mOscillator.Process() * gain;

    for (int c = 0; c < nChans; c++)
    {
      outputs[c][s] = output;
    }
  }
}

void IPlugP5js::OnReset()
{
  mOscillator.SetSampleRate(GetSampleRate());
}
#endif
