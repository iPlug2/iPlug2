#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"

PLUG_CLASS_NAME::PLUG_CLASS_NAME(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  
#if IPLUG_EDITOR
  CreateUI(); // this could be called by superclass - OnCreateUI?
#endif
//  
  PrintDebugInfo();
}

#if IPLUG_DSP
void PLUG_CLASS_NAME::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;

  const int nChans = NOutChansConnected();
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;//mOsc.Process()
    }
  }
}
#endif

#include "IPlugEffect_editor.cpp"
