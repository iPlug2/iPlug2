#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"

PLUG_CLASS_NAME::PLUG_CLASS_NAME(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");

#if defined WEBSOCKET_SERVER && !defined OS_WEB
  CreateServer("/Users/oli/Dev/MyPlugins/Examples/IPlugEffect/build-web", "8001");
#endif
  
  PrintDebugInfo();
}

#if IPLUG_DSP
void PLUG_CLASS_NAME::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);

  const int nChans = NOutChansConnected();
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = outputs[c][s] * gain;
    }
  }
  
  mMeterBallistics.ProcessBlock(outputs, nFrames);
  mScopeBallistics.ProcessBlock(outputs, nFrames);
}

void PLUG_CLASS_NAME::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
}

void PLUG_CLASS_NAME::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void PLUG_CLASS_NAME::OnIdle()
{
  mMeterBallistics.TransmitData(*this);
  mScopeBallistics.TransmitData(*this);
  
#if defined WEBSOCKET_SERVER
  ProcessWebsocketQueue();
#endif
}

#endif

#if IPLUG_EDITOR && !defined NO_IGRAPHICS
#include "IControls.h"
#include "IVKeyboardControl.h"
#include "IPlugEffect_controls.h"

void PLUG_CLASS_NAME::CreateUI()
{  
  IGraphics* pGraphics = MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60, 1.);
  pGraphics->AttachCornerResizer();
  pGraphics->AttachPanelBackground(COLOR_GRAY);

  const int nRows = 4;
  const int nColumns = 4;
  int cellIdx = 0;
  IRECT bounds = pGraphics->GetBounds();
  pGraphics->LoadFont("Roboto-Regular.ttf");
  auto svg1 = pGraphics->LoadSVG(SVGKNOB_FN);
  auto svg2 = pGraphics->LoadSVG(TIGER_FN);
  auto bitmap1 = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
  auto bitmap2 = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);
  
  IColor color;
  #if 1
  pGraphics->AttachControl(new IVMeterControl<2>(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.)), kControlTagMeter);
  pGraphics->AttachControl(new IVScopeControl<>(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.)), kControlTagScope);
  
  
  pGraphics->AttachControl(new IArcControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));
  pGraphics->AttachControl(new IPolyControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));
  pGraphics->AttachControl(new IGradientControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));
  pGraphics->AttachControl(new IMultiPathControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));

  pGraphics->AttachControl(new IVSVGKnob(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), svg1, kGain));
  pGraphics->AttachControl(new IVSVGKnob(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), svg2, kGain));
  #endif

  #if 0
  for (auto i = 0; i < nRows * nColumns; i++) {
    pGraphics->AttachControl(new IBKnobControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), bitmap1, kGain));
  }
  #endif
//  pGraphics->AttachControl(new IBKnobRotaterControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), bitmap2, kGain));
  

//  IVSliderControl* pSlider = new IVSliderControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.).GetMidHPadded(20.));
//
//  pSlider->SetActionFunction([pGraphics, pSlider, this](IControl* pCaller) {
//  for (auto i = 0; i < pGraphics->NControls(); i++) {
//   IVectorBase* pVectorBase = dynamic_cast<IVectorBase*>(pGraphics->GetControl(i));
//   if(pVectorBase)
//     pVectorBase->SetRoundness(pSlider->GetValue());
//     // we can call lambda functions on the plug-in base class here, to avoid down casting
//  }
//  });
//
//  pGraphics->AttachControl(pSlider);
////
//  pGraphics->AttachControl(new IVKnobControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetPadded(-5.), kGain));
//  pGraphics->AttachControl(new IVSwitchControl(*this, bounds.GetGridCell(cellIdx++, nRows, nColumns).GetCentredInside(80.f), -1));

#if 1
  IRECT kbrect = bounds.SubRectVertical(3, 2).GetPadded(-5.);
  pGraphics->AttachControl(new IVKeyboardControl(*this, kbrect, 36, 72));
#endif
  
  AttachGraphics(pGraphics);
  
  pGraphics->HandleMouseOver(true);
  //  pGraphics->EnableLiveEdit(true);
  //  pGraphics->ShowControlBounds(true);
  //  pGraphics->ShowAreaDrawn(true);
}

#endif
