#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "config.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
, mFaustNoise("Noise", "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Noise.dsp",
                     "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Noise.hpp",
                     "/Users/oli/Dev/MyPlugins/IPlug/Extras/IPlugFaust_arch.cpp")
, mFaustOsc("Osc",   "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.dsp",
                     "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.hpp",
                     "/Users/oli/Dev/MyPlugins/IPlug/Extras/IPlugFaust_arch.cpp")
{
  TRACE;
  
  //These Inits will not be necessary when faust .DSP input file is read
  mFaustNoise.Init("import(\"stdfaust.lib\");gain = vslider(\"Gain\", 0, 0., 1, 0.1); process = no.noise * gain, no.noise * gain;");
  mFaustOsc.Init("import(\"stdfaust.lib\");process(l,r) = l + (os.osc(1000)), r + (os.osc(440));");

  mFaustNoise.CreateIPlugParameters(*this);
  
  // these need to be triggered automatically when the files change
  mFaustNoise.CompileCPP();
  mFaustOsc.CompileCPP();

  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaustNoise.ProcessBlock(inputs, outputs, nFrames);
  mFaustOsc.ProcessBlock(outputs, outputs, nFrames);
}

void IPlugEffect::OnReset()
{
  mFaustNoise.SetSampleRate(GetSampleRate());
  mFaustOsc.SetSampleRate(GetSampleRate());
}

void IPlugEffect::OnParamChange(int paramIdx)
{
  mFaustNoise.SetParameterValue(paramIdx, GetParam(paramIdx)->Value());
}

