#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IVMeterControl.h"
#include "config.h"

#include "IPlugEffect_controls.h"

#ifdef OS_MAC
#pragma mark - WATCH OUT IF APP IS SANDBOXED, YOU WON'T FIND ANY FILES HERE
#define SVG_FOLDER "/Users/oli/Dev/VCVRack/Rack/res/ComponentLibrary/"
#define KNOB_FN "resources/img/BefacoBigKnob.svg"
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

  IGraphics* pGraphics = MakeGraphics(*this, PLUG_WIDTH, 600, 60);
  pGraphics->AttachPanelBackground(COLOR_GRAY);

  const int nRows = 2;
  const int nColumns = 2;

  IRECT bounds = pGraphics->GetBounds();
  IColor color;
  //pGraphics->AttachControl(new IArcControl(*this, bounds.GetGridCell(0, nRows, nColumns).GetPadded(-5.), kGain));
 // pGraphics->AttachControl(new IPolyControl(*this, bounds.GetGridCell(1, nRows, nColumns).GetPadded(-5.), -1));

 pMeter1 = new IVMeterControl(*this, IRECT(50, 20, 200, 280), 4, "in L/R", " ", "out L/R", " ");
 //pMeter2 = new IVMeterControl(*this, IRECT(100, 20, 120, 280));
 //pMeter3 = new IVMeterControl(*this, IRECT(150, 20, 170, 280));
 //pMeter4 = new IVMeterControl(*this, IRECT(200, 20, 220, 280));
 //pMeter5 = new IVMeterControl(*this, IRECT(250, 20, 270, 280));
 ((IVMeterControl*) pMeter1)->SetChNameHOffset(0, 10);
 ((IVMeterControl*) pMeter1)->SetChNameHOffset(2, 6);

 ((IVMeterControl*) pMeter1)->SetPeakDropTimeMs(500.0);
 ((IVMeterControl*) pMeter1)->SetDistToTheNextMeter(0.0, 0);
 ((IVMeterControl*) pMeter1)->SetDistToTheNextMeter(0.0, 2, false);

 ((IVMeterControl*) pMeter1)->SetHoldPeaks(false);
 //((IVMeterControl*) pMeter1)->SetDrawPeakRect(false);

 //((IVMeterControl*) pMeter1)->SetDistToTheNextMeter(0.0);
//((IVMeterControl*) pMeter5)->SetOverdriveThreshold(-6.0);

 //((IVMeterControl*) pMeter2)->SetPeakDropTimeMs(1000);
 //((IVMeterControl*) pMeter3)->SetPeakDropTimeMs(1000);
 //((IVMeterControl*) pMeter4)->SetPeakDropTimeMs(1000);
 //((IVMeterControl*) pMeter5)->SetPeakDropTimeMs(1000);
 //
 //((IVMeterControl*) pMeter1)->SetHoldPeaks(false);
 //((IVMeterControl*) pMeter2)->SetHoldPeaks(false);
 //((IVMeterControl*) pMeter3)->SetHoldPeaks(false);
 //((IVMeterControl*) pMeter4)->SetHoldPeaks(false);
 //((IVMeterControl*) pMeter5)->SetHoldPeaks(false);
 /*
 ((IVMeterControl*) pMeter2)->SetMinMaxDisplayValues(0.0, 0.14);
 ((IVMeterControl*) pMeter3)->SetMinMaxDisplayValues(0.0, 2.0);
 ((IVMeterControl*) pMeter4)->SetMinMaxDisplayValues(0.9, 1.5);
 ((IVMeterControl*) pMeter5)->SetMinMaxDisplayValues(2.0, 3.0);
 */

((IVMeterControl*) pMeter1)->SetMinMaxDisplayValues(-60.0, 4.0);//
((IVMeterControl*) pMeter1)->SetDrawLevelMarks(false, 1);
((IVMeterControl*) pMeter1)->SetDrawLevelMarks(false, 3);
//((IVMeterControl*) pMeter5)->SetDisplayInDB(true);
//((IVMeterControl*) pMeter5)->SetOverdriveThreshold(-6.0);
//((IVMeterControl*) pMeter5)->SetMinMaxDisplayValues(-60.0, 3.0);


  pGraphics->AttachControl(pMeter1);
  //pGraphics->AttachControl(pMeter2);
  //pGraphics->AttachControl(pMeter3);
  //pGraphics->AttachControl(pMeter4);
  //pGraphics->AttachControl(pMeter5);


//  for(auto cell = 0; cell < (NRows * NColumns); cell++ )
//  {
//    IRECT cellRect = bounds.GetGridCell(cell, NRows, NColumns);
//    pGraphics->AttachControl(new IVSwitchControl(*this, cellRect, kNoParameter, [pGraphics](IControl* pCaller)
//                                                   {
//                                                     pCaller->SetMEWhenGrayed(true);
//                                                     pCaller->GrayOut(pGraphics->ShowMessageBox("Disable that box control?", "", MB_YESNO) == IDYES);
//                                                   }));
//  }

//  auto svg = pGraphics->LoadSVG(KNOB_FN); // load initial svg, can be a resource or absolute path
//
//  for(auto cell = 0; cell < (NRows * NColumns); cell++ )
//  {
//    IRECT cellRect = bounds.GetGridCell(cell, NRows, NColumns);
//    auto knobControl = new SVGKnob(*this, cellRect, svg, kGain);
//    pGraphics->AttachControl(knobControl);
//  }

  //  auto fileMenuControl = new FileMenu(*this, bounds.GetGridCell(1, NRows, NColumns).SubRectVertical(2, 1).GetVPadded(-20.).GetHPadded(-20.),
//                                           [pGraphics, knobControl](IControl* pCaller)
//                                           {
//                                             WDL_String path;
//                                             dynamic_cast<IDirBrowseControlBase*>(pCaller)->GetSelecteItemPath(path);
//                                             auto svg = pGraphics->LoadSVG(path.Get());
//                                             knobControl->SetSVG(svg);
//                                           },
//                                          DEFAULT_TEXT, ".svg");
//  fileMenuControl->SetPath(SVG_FOLDER);
//  pGraphics->AttachControl(fileMenuControl);


//  IRECT kbrect = bounds.SubRectVertical(2, 1).GetPadded(-5.); // same as joining two cells
//  pGraphics->AttachControl(new IVKeyboardControl(*this, kbrect, 36, 60));

  AttachGraphics(pGraphics);

  pGraphics->HandleMouseOver(true);
  //pGraphics->EnableLiveEdit(true);
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

  //((IVMeterControl*)pMeter1)->ProcessBus(inputs, nFrames);
  //((IVMeterControl*)pMeter2)->ProcessBlock(inputs[0], nFrames);
  //((IVMeterControl*)pMeter3)->ProcessBlock(inputs[0], nFrames);
  //((IVMeterControl*)pMeter4)->ProcessBlock(inputs[0], nFrames);
  //((IVMeterControl*)pMeter5)->ProcessBlock(inputs[0], nFrames);

  const int nChans = NChannelsConnected(ERoute::kOutput);

  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s];// *0.25;
    }
  }

  ((IVMeterControl*)pMeter1)->ProcessInsOuts(inputs, outputs, nFrames);
}
