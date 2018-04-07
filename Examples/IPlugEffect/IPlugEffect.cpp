#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"

#include "config.h"

#define OLBPFRAND() -1. + (2. * rand()/(RAND_MAX+1.) ) // returns random value between -1. and 1.

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;
  
//  IGraphics* pGraphics = MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60);
//  pGraphics->AttachPanelBackground(COLOR_GRAY);
//
//  const int nRows = 2;
//  const int nColumns = 2;
//  IRECT bounds = pGraphics->GetBounds();
//  
//  IRECT cellRect = bounds.GetGridCell(0, nRows, nColumns);
//  pGraphics->AttachControl(new IVSwitchControl(*this, cellRect, kNoParameter, [pGraphics, this](IControl* pCaller)
//                                               {
//                                               }));
  
//  AttachGraphics(pGraphics);
  
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");

  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  ENTER_PARAMS_MUTEX;
  const double gain = GetParam(kGain)->Value() / 100.;
  LEAVE_PARAMS_MUTEX;
  
  const int nChans = NOutChansConnected();
  
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
