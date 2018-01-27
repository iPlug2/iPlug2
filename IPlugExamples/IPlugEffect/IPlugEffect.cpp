#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "config.h"

#include "IPlugEffect_controls.h"

#ifdef OS_OSX
#pragma mark - WATCH OUT IF APP IS SANDBOXED, YOU WON T FIND ANY FILES HERE
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

  IGraphics* pGraphics = MakeGraphics(*this, kWidth, kHeight, 60);
  pGraphics->AttachPanelBackground(COLOR_RED);
  
  const int NRows = 2;
  const int NColumns = 2;

  IRECT bounds = pGraphics->GetBounds();
  IColor color;

  pGraphics->AttachControl(new IArcControl(*this, bounds.GetGridCell(0, NRows, NColumns), kGain));
  pGraphics->AttachControl(new IPolyControl(*this, bounds.GetGridCell(1, NRows, NColumns), -1));

  //for(auto cell = 0; cell < (NRows * NColumns); cell++ )
  //{
  //  IRECT cellRect = bounds.GetGridCell(cell, NRows, NColumns);
  //  color.Randomise();
  //  pGraphics->AttachControl(new IPanelControl(*this, cellRect.GetPadded(-5.), color));
  //}
  
  // lamda function for custom actions on stock IControls
  //pGraphics->AttachControl(new IVSwitchControl(*this, bounds.GetGridCell(0, NRows, NColumns).GetPadded(-20), kNoParameter, [pGraphics](IControl* pCaller)
  //{
  //  pCaller->SetMEWhenGrayed(true);
  //  pCaller->GrayOut(pGraphics->ShowMessageBox("Disable that box control?", "", MB_YESNO) == IDYES);
  //}));

  //auto svg = pGraphics->LoadSVG(KNOB_FN); // load initial svg, can be a resource or absolute path
  //auto knobControl = new SVGKnob(*this, bounds.GetGridCell(1, NRows, NColumns).SubRectVertical(2, 0).GetPadded(-5.), svg, kGain);
  //auto fileMenuControl = new FileMenu(*this, bounds.GetGridCell(1, NRows, NColumns).SubRectVertical(2, 1).GetVPadded(-20.).GetHPadded(-20.),
  //                                         [pGraphics, knobControl](IControl* pCaller)
  //                                         {
  //                                           WDL_String path;
  //                                           dynamic_cast<IDirBrowseControlBase*>(pCaller)->GetSelecteItemPath(path);
  //                                           auto svg = pGraphics->LoadSVG(path.Get());
  //                                           knobControl->SetSVG(svg);
  //                                         },
  //                                        DEFAULT_TEXT, ".svg");
  //fileMenuControl->SetPath(SVG_FOLDER);
  //
  //pGraphics->AttachControl(fileMenuControl);

  //IRECT kbrect = bounds.SubRectVertical(2, 1); // same as joining two cells
  //pGraphics->AttachControl(new IVKeyboardControl(*this, kbrect, 36, 72));
  //pGraphics->AttachControl(knobControl);

  AttachGraphics(pGraphics);
  
//  pGraphics->EnableLiveEdit(true);
//  pGraphics->ShowControlBounds(true);
//  pGraphics->ShowAreaDrawn(true);

  PrintDebugInfo();

  MakeDefaultPreset("-", kNumPrograms);
}

void IPlugEffect::ProcessBlock(double** inputs, double** outputs, int nFrames)
{
  mParams_mutex.Enter();
  const double gain = GetParam(kGain)->Value() / 100.;
  mParams_mutex.Leave();
  
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1 * gain;
    *out2 = *in2 * gain;
  }
}
