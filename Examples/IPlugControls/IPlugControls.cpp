#include "IPlugControls.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IPlugPaths.h"

IPlugControls::IPlugControls(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    if(pGraphics->NControls())
    {
      //Could handle new layout here
      return;
    }
    
//    pGraphics->EnableLiveEdit(true);
    pGraphics->HandleMouseOver(true);
    pGraphics->AttachCornerResizer(kUIResizerScale, true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    
    IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    pGraphics->LoadFont(ROBOTTO_FN);
    const IBitmap bitmap1 = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
    const IBitmap bitmap2 = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);
    const IText bigLabel {24, COLOR_WHITE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignNear, IText::kVAlignTop, 0};
    const IText buttonLabels {14, COLOR_BLACK, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignMiddle, 0};

//    auto svg1 = pGraphics->LoadSVG(SVGKNOB_FN);
    
//    pGraphics->AttachControl(new IVMeterControl<2>(*this, nextCell()), kControlTagMeter);
//    pGraphics->AttachControl(new IVScopeControl<>(*this, nextCell()), kControlTagScope);
//    pGraphics->AttachControl(new IVSVGKnob(*this, nextCell(), svg1, kGain));
    pGraphics->AttachControl(new ITextControl(*this, b.GetGridCell(0, 4, 1), "Bitmap Controls", bigLabel));
    pGraphics->AttachControl(new IBKnobControl(*this, b.GetGridCell(0, 4, 4).GetPadded(-5.), bitmap1, kGain));
    pGraphics->AttachControl(new IBKnobRotaterControl(*this, b.GetGridCell(1, 4, 4).GetPadded(-5.), bitmap2, kGain));
    pGraphics->AttachControl(new IBSwitchControl(*this, b.GetGridCell(2, 4, 4), bitmap1));
    pGraphics->AttachControl(new IBButtonControl(*this, b.GetGridCell(3, 4, 4), bitmap1));

    pGraphics->AttachControl(new ITextControl(*this, b.GetGridCell(1, 4, 1), "Vector Controls", bigLabel));
    pGraphics->AttachControl(new IVKnobControl(*this, b.GetGridCell(4, 4, 4).GetCentredInside(100.), kGain));
    pGraphics->AttachControl(new IVSliderControl(*this, b.GetGridCell(5, 4, 4).GetGridCell(0, 1, 3)));
    pGraphics->AttachControl(new IVSliderControl(*this, b.GetGridCell(5, 4, 4).GetGridCell(3, 3, 2), kNoParameter, DEFAULT_SPEC, kHorizontal));
    pGraphics->AttachControl(new IVSwitchControl(*this, b.GetGridCell(6, 4, 4).GetCentredInside(30.)), 0);
    
    auto button1action = [](IControl* pCaller) {
      FlashCircleClickActionFunc(pCaller);
      
      IVColorSpec spec;
      spec.mBGColor = COLOR_RED;
      
      IGraphics* pGraphics = pCaller->GetUI();
      
      pGraphics->StyleAllVectorControls(true, true, false, 10., 20., 3., spec);

      DBGMSG("%i\n", pCaller->GetUI()->ShowMessageBox("Str", "Caption", MB_YESNO));
    };
    
    auto button2action = [](IControl* pCaller) {
      FlashCircleClickActionFunc(pCaller);
      WDL_String file, path;
      pCaller->GetUI()->PromptForFile(file, path);
      DBGMSG("%s %s\n", file.Get(), path.Get());
    };
    
    auto button3action = [](IControl* pCaller) {
      FlashCircleClickActionFunc(pCaller);
      WDL_String dir;
      pCaller->GetUI()->PromptForDirectory(dir);
      DBGMSG("%s\n", dir.Get());
    };
    
    pGraphics->AttachControl(new IVButtonControl(*this, b.GetGridCell(7, 4, 4).GetGridCell(0, 2, 2), button1action, "Trigger Message Box", buttonLabels));
    pGraphics->AttachControl(new IVButtonControl(*this, b.GetGridCell(7, 4, 4).GetGridCell(1, 2, 2), button2action, "Trigger open file dialog", buttonLabels));
    pGraphics->AttachControl(new IVButtonControl(*this, b.GetGridCell(7, 4, 4).GetGridCell(3, 2, 2), button3action, "Trigger open dir dialog", buttonLabels));

    pGraphics->AttachControl(new ITextControl(*this, b.GetGridCell(2, 4, 1), "Text Controls", bigLabel));
    pGraphics->AttachControl(new ICaptionControl(*this, b.GetGridCell(8, 4, 4).GetMidVPadded(20.), kGain, IText(50)));

    pGraphics->AttachControl(new ITextControl(*this, b.GetGridCell(3, 4, 1), "Misc Controls", bigLabel));
    pGraphics->AttachControl(new IColorPickerControl(*this, b.GetGridCell(12, 4, 4).GetCentredInside(150.)));
    pGraphics->AttachControl(new IVKeyboardControl(*this, b.GetGridCell(13, 4, 4).Union(b.GetGridCell(14, 4, 4)).GetPadded(-40), 36, 72));
  };
#endif
}

#if IPLUG_DSP
void IPlugControls::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
