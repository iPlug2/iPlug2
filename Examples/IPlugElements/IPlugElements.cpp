#include "IPlugElements.h"
#include "IPlug_include_in_plug_src.h"

#include "ElementsView.h"

IPlugElements::IPlugElements(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
}

void IPlugElements::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}

void* IPlugElements::OpenWindow(void* pParent)
{
  mView = make_view(pParent);
}
