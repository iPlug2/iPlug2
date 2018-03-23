#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "config.h"

#include "IPlugEffect_controls.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
, mFaustGen("Gain", "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Gain.dsp",
                    "/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/Gain.cpp")
{
  TRACE;
  
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
//  ENTER_PARAMS_MUTEX;
//  const double gain = GetParam(kGain)->Value() / 100.;
//  LEAVE_PARAMS_MUTEX;
//
//  const int nChans = NOutChansConnected();
//
//  for (auto s = 0; s < nFrames; s++) {
//    for (auto c = 0; c < nChans; c++) {
//      outputs[c][s] = inputs[c][s] * gain;
//    }
//  }
//
  mFaustGen.ProcessBlock(inputs, outputs, nFrames);
}
