#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "config.h"

#include "IPlugEffect_controls.h"

#ifdef OS_MAC
#pragma mark - WATCH OUT IF APP IS SANDBOXED, YOU WON'T FIND ANY FILES HERE
#define SVG_FOLDER "/Users/oli/Dev/VCVRack/Rack/res/ComponentLibrary/"
#define KNOB_FN "resources/img/BefacoBigKnob.svg"
#define TIGER_FN "resources/img/23.svg"
#else
#define SVG_FOLDER "C:\\Program Files\\VCV\\Rack\\res\\ComponentLibrary\\"
#define KNOB_FN "C:\\Program Files\\VCV\\Rack\\res\\ComponentLibrary\\BefacoBigKnob.svg"
#endif

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;
  
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#ifndef NO_IGRAPHICS
  
  IGraphics* pGraphics = MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60);
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  
  const int nRows = 4;
  const int nColumns = 2;

  IRECT bounds = pGraphics->GetBounds();
  IColor color;

  pMeter = new IVMeterControl(*this, bounds.GetPadded(-20), 2, "left", "right");
  pGraphics->AttachControl(pMeter);
//  IRECT kbrect = bounds.SubRectVertical(2, 1).GetPadded(-5.); // same as joining two cells
//  pGraphics->AttachControl(new IVKeyboardControl(*this, kbrect, 36, 60));

  AttachGraphics(pGraphics);
  
  pGraphics->HandleMouseOver(true);
//  pGraphics->EnableLiveEdit(true);
  pGraphics->ShowControlBounds(true);
//  pGraphics->ShowAreaDrawn(true);

#endif
  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  ENTER_PARAMS_MUTEX;
  const double gain = GetParam(kGain)->Value() / 100.;
  LEAVE_PARAMS_MUTEX;
  
  const int nChans = NChannelsConnected(ERoute::kOutput);
  
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
  pMeter->ProcessBus(inputs, nFrames, 2, 0, 0);
}
