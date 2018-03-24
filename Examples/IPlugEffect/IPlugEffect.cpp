#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "config.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
, mFaustNoise("Noise",  1, "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Noise.dsp")
, mFaustOsc1("Osc",     1, "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.dsp")
//, mFaustOsc2("Osc",     1, "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Osc.dsp")
{
  TRACE;
  
  //These Inits will not be necessary when faust .DSP input file is read
  mFaustNoise.Init("import(\"stdfaust.lib\");gain = vslider(\"Gain\", 0, 0., 1, 0.1); process = no.noise * gain, no.noise * gain;");
  mFaustOsc1.Init("import(\"stdfaust.lib\");process(l,r) = l + (os.osc(1000)), r + (os.osc(440));");

  mFaustNoise.CreateIPlugParameters(*this);
  
  // calling this will compile all .dsp files to a single FaustCode.hpp in the same folder, which will be included in the release build
  FaustGen::CompileCPP();

  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  //mFaustNoise.ProcessBlock(inputs, outputs, nFrames);
  mFaustOsc1.ProcessBlock(inputs, outputs, nFrames);
}

void IPlugEffect::OnReset()
{
  mFaustNoise.SetSampleRate(GetSampleRate());
  mFaustOsc1.SetSampleRate(GetSampleRate());
}

void IPlugEffect::OnParamChange(int paramIdx)
{
  mFaustNoise.SetParameterValue(paramIdx, GetParam(paramIdx)->Value());
}

