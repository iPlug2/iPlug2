#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "config.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;

  mNoise.Init(5000);
  mOsc1.Init(5000);
  mOsc2.Init(5000);
  mNoise.CreateIPlugParameters(*this);
  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  //mNoise.ProcessBlock(inputs, outputs, nFrames);
  mOsc1.ProcessBlock(inputs, outputs, nFrames);
}

void IPlugEffect::OnReset()
{
  mNoise.SetSampleRate(GetSampleRate());
  mOsc1.SetSampleRate(GetSampleRate());
  mOsc2.SetSampleRate(GetSampleRate());
}

void IPlugEffect::OnParamChange(int paramIdx)
{
  mNoise.SetParameterValue(paramIdx, GetParam(paramIdx)->Value());
}

