#include "IPlugControls.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IPlugPaths.h"
#include "IconsForkAwesome.h"

class FileBrowser : public IDirBrowseControlBase
{
private:
  WDL_String mLabel;
  IBitmap mBitmap;
public:
  FileBrowser(IRECT bounds)
  : IDirBrowseControlBase(bounds, ".png")
  {
    WDL_String path;
//    DesktopPath(path);
    path.Set(__FILE__);
    path.remove_filepart();
#ifdef OS_WIN
    path.Append("\\resources\\img\\");
#else
    path.Append("/resources/img/");
#endif
    AddPath(path.Get(), "");
    
    mLabel.Set("Click here to browse png files...");
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_TRANSLUCENT, mRECT);
    
    IRECT labelRect = mRECT.GetFromBottom(mText.mSize);
    IRECT bmpRect = mRECT.GetReducedFromBottom(mText.mSize);

    if(mBitmap.GetAPIBitmap())
    {
      //if stacked frames, don't try and fit the whole bitmap to the bounds
      if(mBitmap.N())
        g.DrawBitmap(mBitmap, bmpRect, 1);
      else
        g.DrawFittedBitmap(mBitmap, bmpRect);
    }
    
    g.FillRect(COLOR_WHITE, labelRect);
    g.DrawText(mText, mLabel.Get(), labelRect);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    SetUpMenu();
    
    GetUI()->CreatePopupMenu(*this, mMainMenu, x, y);
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if(pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();
      WDL_String* pStr = mFiles.Get(pItem->GetTag());
      mLabel.Set(pStr);
      mBitmap = GetUI()->LoadBitmap(pStr->Get());
      SetTooltip(pStr->Get());
      SetDirty(false);
    }
  }
};

IPlugControls::IPlugControls(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kMode)->InitEnum("Mode", 0, 4, "", IParam::kFlagsNone, "", "one", "two", "three", "four");
  GetParam(kFreq1)->InitDouble("Freq 1 - X", 50., 0., 100.0, 0.01, "%");
  GetParam(kFreq2)->InitDouble("Freq 2 - Y", 50., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
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
    pGraphics->EnableTooltips(true);
    pGraphics->AttachTextEntryControl();
    
    IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("ForkAwesome", FONT_ICON_FILE_NAME_FK);
    
    const IBitmap bitmap1 = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
    const IBitmap bitmap2 = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);
    const IBitmap switchBitmap = pGraphics->LoadBitmap(PNGSWITCH_FN, 2, true);
    const IBitmap buttonBitmap = pGraphics->LoadBitmap(PNGBUTTON_FN, 10);

    const ISVG vectorknob = pGraphics->LoadSVG(SVGKNOBROTATE_FN);
    
    const IVStyle style {
      true, // Show label
      true, // Show value
      {
        DEFAULT_BGCOLOR, // Background
        DEFAULT_FGCOLOR, // Foreground
        DEFAULT_PRCOLOR, // Pressed
        COLOR_BLACK, // Frame
        DEFAULT_HLCOLOR, // Highlight
        DEFAULT_SHCOLOR, // Shadow
        DEFAULT_X1COLOR, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(12.f, IText::kAlignCenter) // Label text
    };
    
    const IText forkAwesomeText {24.f, "ForkAwesome"};
    const IText bigLabel {24, COLOR_WHITE, "Roboto-Regular", IText::kAlignNear, IText::kVAlignTop, 0};
    const IText buttonLabels {14, COLOR_BLACK, "Roboto-Regular", IText::kAlignCenter, IText::kVAlignMiddle, 0};

    
    const int nRows = 5;
    const int nCols = 8;
    
    int cellIdx = -1;
    
    auto nextCell = [&](){
      return b.GetGridCell(++cellIdx, nRows, nCols).GetPadded(-5.);
    };

    auto sameCell = [&](){
      return b.GetGridCell(cellIdx, nRows, nCols).GetPadded(-5.);
    };
    
    
    auto AddLabel = [&](const char* label){
      pGraphics->AttachControl(new ITextControl(nextCell().GetFromTop(20.f), label, style.labelText));
    };
  
    AddLabel("IBKnobControl");
    pGraphics->AttachControl(new IBKnobControl(sameCell().GetPadded(-5.), bitmap1, kGain));
    AddLabel("IBKnobRotaterControl");
    pGraphics->AttachControl(new IBKnobRotaterControl(sameCell().GetPadded(-5.), bitmap2, kGain));
    AddLabel("IBSwitchControl");
    pGraphics->AttachControl(new IBSwitchControl(sameCell(), switchBitmap));
    AddLabel("IBButtonControl");
    pGraphics->AttachControl(new IBButtonControl(sameCell(), buttonBitmap, [](IControl* pCaller) {
      pCaller->SetAnimation([](IControl* pCaller){
        auto progress = pCaller->GetAnimationProgress();
        if(progress > 1.) {
          pCaller->OnEndAnimation();
          return;
        }
        pCaller->SetValue(Clip(progress + .5, 0., 1.));
      }, 100);
    }));

    auto button1action = [&](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      pCaller->GetUI()->ShowMessageBox("Message Title", "Message", kMB_YESNO, [&](EMsgBoxResult result) {
                                                      WDL_String str;
                                                      str.SetFormatted(32, "%s pressed", kMessageResultStrs[result]);
                                                      dynamic_cast<ITextControl*>(GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(str.Get());
                                                    });
    };

    pGraphics->AttachControl(new IVKnobControl(nextCell().GetCentredInside(110.), kGain, "IVKnobControl", style, true));
    pGraphics->AttachControl(new IVSliderControl(nextCell().GetCentredInside(110.), kGain, "IVSliderControl", style, true));
    pGraphics->AttachControl(new IVRangeSliderControl(nextCell().GetCentredInside(110.), kFreq1, kFreq2, "IVRangeSliderControl", style, kVertical, true, 10.f, 50.f));

    pGraphics->AttachControl(new IVButtonControl(nextCell().GetCentredInside(110.), button1action, "IVButtonControl", style, false), kCtrlTagVectorButton);
    AddLabel("IVButtonControl 2");
    pGraphics->AttachControl(new IVButtonControl(sameCell().GetCentredInside(110.), button1action, "Label in button", style, true));

    AddLabel("IVButtonControl 3");
    pGraphics->AttachControl(new IVButtonControl(sameCell().GetCentredInside(110.), [](IControl* pCaller){
      
      SplashClickActionFunc(pCaller);

      static IPopupMenu menu {{"One", "Two", "Three"}, [](int indexInMenu, IPopupMenu::Item* itemChosen) {
        
        }
      };
      
      pCaller->GetUI()->CreatePopupMenu(*pCaller, menu, pCaller->GetRECT());
      
    }, ICON_FK_BOMB, style.WithLabelText(forkAwesomeText), true));

    pGraphics->AttachControl(new IVSwitchControl(nextCell().GetCentredInside(110.), kMode, "IVSwitchControl", style.WithValueText(IText(36.f, IText::kAlignCenter))));

    pGraphics->AttachControl(new IVToggleControl(nextCell().GetCentredInside(110.), SplashClickActionFunc, "", ICON_FK_CHECK, "IVToggleControl", style.WithValueText(forkAwesomeText)));

    pGraphics->AttachControl(new IVRadioButtonControl(nextCell().GetCentredInside(110.), [](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      dynamic_cast<IVButtonControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagVectorButton))->SetShape((IVShape) dynamic_cast<IVRadioButtonControl*>(pCaller)->GetSelectedIdx());

    }, {"One", "Two", "Three"}, "IVRadioButtonControl", style, kVShapeCircle, 5.f));

    pGraphics->AttachControl(new IVXYPadControl(nextCell(), {kFreq1, kFreq2}, "IVXYPadControl", style));

    AddLabel("ITextControl");
    pGraphics->AttachControl(new ITextControl(sameCell().GetMidVPadded(20.f), "Result...", DEFAULT_TEXT, COLOR_LIGHT_GRAY), kCtrlTagDialogResult);

    AddLabel("ITextToggleControl");
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 0, 3, 3), nullptr, ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, forkAwesomeText));
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 1, 3, 3), nullptr, ICON_FK_CIRCLE_O, ICON_FK_CHECK_CIRCLE, forkAwesomeText));
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 2, 3, 3), nullptr, ICON_FK_PLUS_SQUARE, ICON_FK_MINUS_SQUARE, forkAwesomeText));


    pGraphics->AttachControl(new IVMultiSliderControl<4>(nextCell(), style));

    pGraphics->AttachControl(new IVMeterControl<2>(nextCell(), style), kCtrlTagMeter);
    pGraphics->AttachControl(new IVScopeControl<2>(nextCell(), style.WithColor(kFG, COLOR_BLACK)), kCtrlTagScope);
    
    pGraphics->AttachControl(new ISVGKnob(nextCell().GetCentredInside(100), vectorknob, kGain));
    

    pGraphics->AttachControl(new IVSliderControl(nextCell().GetGridCell(0, 0, 3, 1), [](IControl* pCaller) {
      dynamic_cast<IVButtonControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagVectorButton))->SetRoundness(pCaller->GetValue());
    }, "Roundness", style, true, kHorizontal));
    
    pGraphics->AttachControl(new IVSliderControl(sameCell().GetGridCell(1, 0, 3, 1), [](IControl* pCaller) {
      dynamic_cast<IVButtonControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagVectorButton))->SetAngle(pCaller->GetValue() * 360.);
    }, "Angle", style, true, kHorizontal));
    
    IRECT cell = nextCell().Union(nextCell()).Union(nextCell());
    for(int i = 0; i < kNumDefaultVColors; i++)
    {
      IRECT r = cell.GetGridCell(i, 3, 3);
      pGraphics->AttachControl(new IVButtonControl(r, [](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        IColor currentColor = dynamic_cast<IVButtonControl*>(pCaller)->GetColor(kFG);
        pCaller->GetUI()->PromptForColor(currentColor, "", [=](const IColor& result) {
                                           dynamic_cast<IVButtonControl*>(pCaller)->SetColor(kFG, result);
                                         });
      }, kVColorStrs[i], style.WithColor(kFG, DEFAULT_SPEC.mColors[i]).WithDrawFrame(false).WithDrawShadows(false)));
    }
    
    
    
//
//    auto button2action = [](IControl* pCaller) {
//      SplashClickActionFunc(pCaller);
//      WDL_String file, path;
//      pCaller->GetUI()->PromptForFile(file, path);
//      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(file.Get());
//    };
//
//    auto button3action = [](IControl* pCaller) {
//      SplashClickActionFunc(pCaller);
//      WDL_String dir;
//      pCaller->GetUI()->PromptForDirectory(dir);
//      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(dir.Get());
//    };
//
//    pGraphics->AttachControl(new IVButtonControl(b.GetGridCell(9, nRows, nCols).GetGridCell(1, 4, 1), button2action, "Trigger open file dialog"));
//    pGraphics->AttachControl(new IVButtonControl(b.GetGridCell(9, nRows, nCols).GetGridCell(2, 4, 1), button3action, "Trigger open directory dialog"));

//    pGraphics->AttachControl(pLabel = new ITextControl(b.GetGridCell(2, nRows, 1), "Text Controls", bigLabel));
//    pLabel->SetBoundsBasedOnTextDimensions();
//
    pGraphics->AttachControl(new ICaptionControl(nextCell().GetMidVPadded(20.), kGain, style.labelText.WithColors(COLOR_RED, COLOR_BLACK, COLOR_RED), false));
//
//    pGraphics->AttachControl(pLabel = new ITextControl(b.GetGridCell(3, nRows, 1), "Misc Controls", bigLabel));
//    pLabel->SetBoundsBasedOnTextDimensions();
//
//    pGraphics->AttachControl(new FileBrowser(b.GetGridCell(15, nRows, nCols).Union(b.GetGridCell(16, nRows, nCols)).GetPadded(-25)));
//    pGraphics->AttachControl(new IVKeyboardControl(b.GetGridCell(17, nRows, nCols).Union(b.GetGridCell(18, nRows, nCols)).GetPadded(-25), 36, 72));
//    pGraphics->AttachControl(new IColorPickerControl(b.GetGridCell(12, nRows, nCols).GetCentredInside(150.)));


  };
#endif
}

#if IPLUG_DSP
void IPlugControls::OnIdle()
{
  mScopeBallistics.TransmitData(*this);
  mMeterBallistics.TransmitData(*this);
}

void IPlugControls::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double phaseIncr1 = GetParam(kFreq1)->Value() * 0.00001;
  const double phaseIncr2 = GetParam(kFreq2)->Value() * 0.00001;

  for (int s = 0; s < nFrames; s++) {
    static double phase1 = 0.;
    static double phase2 = 0.;

    outputs[0][s] = cos(phase1 += phaseIncr1);
    outputs[1][s] = sin(phase2 += phaseIncr2);
  }
  
  mScopeBallistics.ProcessBlock(outputs, nFrames);
  mMeterBallistics.ProcessBlock(outputs, nFrames);

  for (int s = 0; s < nFrames; s++) {
    outputs[0][s] = 0.;
    outputs[1][s] = 0.;
  }
}
#endif
