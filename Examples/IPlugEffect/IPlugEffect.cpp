#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "config.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
, mFaustGain("Gain", "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Gain.dsp",
                     "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Gain.hpp",
                     "/Users/oli/Dev/MyPlugins/IPlug/Extras/IPlugFaust_arch.cpp")
{
  TRACE;
  mFaustGain.Init("import(\"stdfaust.lib\");process = _, no.noise * (vslider(\"Gain\", 0, 0., 1, 0.1));");
  mFaustGain.CreateIPlugParameters(*this);
  mFaustGain.CompileCPP();
  
  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaustGain.ProcessBlock(inputs, outputs, nFrames);
}

void IPlugEffect::OnReset()
{
  mFaustGain.SetSampleRate(GetSampleRate());
}

void IPlugEffect::OnParamChange(int paramIdx)
{
  mFaustGain.SetParameterValue(paramIdx, GetParam(paramIdx)->Value());
}

