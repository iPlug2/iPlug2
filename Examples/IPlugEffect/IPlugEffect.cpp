#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "config.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
, mFaustGain("Gain", "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Gain.dsp",
                     "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Gain.hpp",
                     "/Users/oli/Dev/MyPlugins/IPlug/Extras/IPlugFaust_arch.cpp")
, mFaustOsc("Osc",   "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.dsp",
                     "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.hpp",
                     "/Users/oli/Dev/MyPlugins/IPlug/Extras/IPlugFaust_arch.cpp")
{
  TRACE;
  
  //These Inits will not be necessary when faust .DSP input file is read
  mFaustGain.Init("import(\"stdfaust.lib\");process = _, no.noise * (vslider(\"Gain\", 0, 0., 1, 0.1));");
  mFaustOsc.Init("import(\"stdfaust.lib\");process = _, _ + os.osc(440);");

  mFaustGain.CreateIPlugParameters(*this);
  
  // these need to be triggered automatically when the files change
  mFaustGain.CompileCPP();
  mFaustOsc.CompileCPP();

  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaustGain.ProcessBlock(inputs, outputs, nFrames);
  mFaustOsc.ProcessBlock(inputs, outputs, nFrames);
}

void IPlugEffect::OnReset()
{
  mFaustGain.SetSampleRate(GetSampleRate());
  mFaustOsc.SetSampleRate(GetSampleRate());
}

void IPlugEffect::OnParamChange(int paramIdx)
{
  mFaustGain.SetParameterValue(paramIdx, GetParam(paramIdx)->Value());
}

