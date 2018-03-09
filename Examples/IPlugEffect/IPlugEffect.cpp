#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IVKeyboardControl.h"

#include "config.h"

#include "IPlugEffect_controls.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;
  
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#ifndef NO_IGRAPHICS
  
  IGraphics* pGraphics = MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60);
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  
  const int nRows = 4;
  const int nColumns = 4;
  int cellIdx = 0;
  IRECT bounds = pGraphics->GetBounds();
  IColor color;
  
  pGraphics->AttachControl(new IArcControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));
  pGraphics->AttachControl(new IPolyControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));

  pGraphics->AttachControl(new IGradientControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));
  pGraphics->AttachControl(new IMultiPathControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));

  auto svg1 = pGraphics->LoadSVG(SVGKNOB_FN);
  auto svg2 = pGraphics->LoadSVG(TIGER_FN);
  pGraphics->AttachControl(new IVSVGKnob(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), svg1, kGain));
  pGraphics->AttachControl(new IVSVGKnob(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), svg2, kGain));

  auto bitmap1 = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
  auto bitmap2 = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);

  pGraphics->AttachControl(new IBKnobControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), bitmap1, kGain));
  pGraphics->AttachControl(new IBKnobRotaterControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), bitmap2, kGain));

  pGraphics->AttachControl(new IVSliderControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.).GetMidHPadded(20.), kGain));

  IRECT kbrect = bounds.SubRectVertical(4, 3).GetPadded(-5.);
  pGraphics->AttachControl(new IVKeyboardControl(*this, kbrect, 36, 72));

  AttachGraphics(pGraphics);
  
  pGraphics->HandleMouseOver(true);
//  pGraphics->EnableLiveEdit(true);
//  pGraphics->ShowControlBounds(true);
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
}
