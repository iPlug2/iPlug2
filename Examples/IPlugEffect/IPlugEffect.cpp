#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  
#if IPLUG_EDITOR
  CreateUI(); // this could be called by superclass - OnCreateUI?
#endif
  
  PrintDebugInfo();
}

#if IPLUG_DSP
void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;

//  const int nChans = NOutChansConnected();
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < 1; c++) {
      outputs[c][s] = mOsc.Process() * gain;
    }
  }

 // mMeter->ProcessBus(outputs, nFrames);
}
#endif

#include "IPlugEffect_editor.cpp"
