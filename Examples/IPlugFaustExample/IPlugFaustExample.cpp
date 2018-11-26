#include "IPlugFaustExample.h"
#include "IPlug_include_in_plug_src.h"

IPlugFaustExample::IPlugFaustExample(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(1, 1, instanceInfo)
{
  TRACE;
  GetParam(0)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  
  mDistortion.SetMaxChannelCount(0, 2);
  mDistortion.CompileCPP();
  mDistortion.SetAutoRecompile(true);
}

#if IPLUG_DSP
void IPlugFaustExample::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDistortion.ProcessBlock(inputs, outputs, nFrames);
}

void IPlugFaustExample::OnReset()
{
  mDistortion.SetSampleRate(GetSampleRate());
}

void IPlugFaustExample::OnParamChange(int paramIdx)
{
  switch (paramIdx) {
    case 0: mDistortion.SetParameterValueNormalised(paramIdx, GetParam(paramIdx)->Value() / 100.); break;
    default: break;
  }
}
#endif
