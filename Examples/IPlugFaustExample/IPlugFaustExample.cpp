#include "IPlugFaustExample.h"
#include "IPlug_include_in_plug_src.h"

IPlugFaustExample::IPlugFaustExample(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(1, 1, instanceInfo)
{
  GetParam(0)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  
  mFaustProcessor.SetMaxChannelCount(0, 2);
  mFaustProcessor.CompileCPP();
  mFaustProcessor.SetAutoRecompile(true);
}

void IPlugFaustExample::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaustProcessor.ProcessBlock(inputs, outputs, nFrames);
}

void IPlugFaustExample::OnReset()
{
  mFaustProcessor.SetSampleRate(GetSampleRate());
}

void IPlugFaustExample::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case 0: mFaustProcessor.SetParameterValueNormalised(paramIdx, GetParam(paramIdx)->Value() / 100.); break;
    default: break;
  }
}
