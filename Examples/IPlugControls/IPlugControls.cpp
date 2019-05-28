#include "IPlugControls.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IPlugPaths.h"
#include "IconsForkAwesome.h"

class IVCustomControl : public IControl
                      , public IVectorBase
{
public:
  IVCustomControl(IRECT bounds, const char* label, const IVStyle& style)
  : IControl(bounds)
  , IVectorBase(style)
  {
    AttachIControl(this, label);
  }

  void OnInit() override
  {
    mValueStr.Set("Test");
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    DrawValue(g, mMouseIsOver);
  }
  
  virtual void DrawWidget(IGraphics& g) override
  {
    g.FillRect(GetColor(kFG), mWidgetBounds);
  }
  
  void OnResize() override
  {
    SetTargetRECT(CalculateRects(mRECT));
  }
};

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
  GetParam(kFreq1)->InitDouble("Freq 1 - X", 0.5, 0., 2, 0.01, "Hz");
  GetParam(kFreq2)->InitDouble("Freq 2 - Y", 0.5, 0., 2, 0.01, "Hz");

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
    pGraphics->AttachPopupMenuControl();
    
    IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("ForkAwesome", FORK_AWESOME_FN);
    
    const IBitmap bitmap1 = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
    const IBitmap bitmap2 = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);
    const IBitmap switchBitmap = pGraphics->LoadBitmap(PNGSWITCH_FN, 2, true);
    const IBitmap buttonBitmap = pGraphics->LoadBitmap(PNGBUTTON_FN, 10);

    const ISVG knobSVG = pGraphics->LoadSVG(SVGKNOBROTATE_FN);
    
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
  
    
    AddLabel("ITextControl");
    pGraphics->AttachControl(new ITextControl(sameCell().GetMidVPadded(20.f), "Result...", DEFAULT_TEXT, COLOR_LIGHT_GRAY), kCtrlTagDialogResult);
    
    AddLabel("ITextToggleControl");
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 0, 3, 3), nullptr, ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, forkAwesomeText));
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 1, 3, 3), nullptr, ICON_FK_CIRCLE_O, ICON_FK_CHECK_CIRCLE, forkAwesomeText));
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 2, 3, 3), nullptr, ICON_FK_PLUS_SQUARE, ICON_FK_MINUS_SQUARE, forkAwesomeText));

    AddLabel("ICaptionControl");
    pGraphics->AttachControl(new ICaptionControl(sameCell().FracRectVertical(0.5, true).GetMidVPadded(10.f), kGain, IText(24.f), DEFAULT_FGCOLOR, false));
    pGraphics->AttachControl(new ICaptionControl(sameCell().FracRectVertical(0.5, false).GetMidVPadded(10.f), kMode, IText(24.f), DEFAULT_FGCOLOR, false));

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
    
    AddLabel("ISVGKnob");
    pGraphics->AttachControl(new ISVGKnob(sameCell().GetCentredInside(100), knobSVG, kGain));

    auto button1action = [&](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      pCaller->GetUI()->ShowMessageBox("Message Title", "Message", kMB_YESNO, [&](EMsgBoxResult result) {
                                                      WDL_String str;
                                                      str.SetFormatted(32, "%s pressed", kMessageResultStrs[result]);
                                                      dynamic_cast<ITextControl*>(GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(str.Get());
                                                    });
    };

    pGraphics->AttachControl(new IVKnobControl(nextCell().GetCentredInside(110.), kGain, "IVKnobControl", style, true), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVSliderControl(nextCell().GetCentredInside(110.), kGain, "IVSliderControl", style, true), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVRangeSliderControl(nextCell().GetCentredInside(110.), kFreq1, kFreq2, "IVRangeSliderControl", style, kVertical, true, 10.f, 50.f), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVButtonControl(nextCell().GetCentredInside(110.), button1action, "IVButtonControl", style, false), kCtrlTagVectorButton, "vcontrols");
    AddLabel("IVButtonControl 2");
    pGraphics->AttachControl(new IVButtonControl(sameCell().GetCentredInside(110.), button1action, "Label in button", style, true), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVButtonControl(nextCell().GetCentredInside(110.), [](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      static IPopupMenu menu {{"one", "two", "three"}, [=](int indexInMenu, IPopupMenu::Item* itemChosen) {
          if(itemChosen)
            dynamic_cast<IVButtonControl*>(pCaller)->SetValueStr(itemChosen->GetText());
        }
      };
      
      float x, y;
      pCaller->GetUI()->GetMouseDownPoint(x, y);
      pCaller->GetUI()->CreatePopupMenu(*pCaller, menu, x, y);
      
    }, "IVButtonControl 3", style.WithValueText(IText(36.f, IText::kVAlignMiddle)),  false, true), kNoTag, "vcontrols");
    dynamic_cast<IVButtonControl*>(pGraphics->GetControl(pGraphics->NControls()-1))->SetValueStr("one");
    
    pGraphics->AttachControl(new IVSwitchControl(nextCell().GetCentredInside(110.), kMode, "IVSwitchControl", style.WithValueText(IText(36.f, IText::kAlignCenter))), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVToggleControl(nextCell().GetCentredInside(110.), SplashClickActionFunc, "", ICON_FK_CHECK, "IVToggleControl", style.WithValueText(forkAwesomeText)), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVRadioButtonControl(nextCell().GetCentredInside(110.), [](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      dynamic_cast<IVButtonControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagVectorButton))->SetShape((IVShape) dynamic_cast<IVRadioButtonControl*>(pCaller)->GetSelectedIdx());

    }, {"One", "Two", "Three"}, "IVRadioButtonControl", style, kVShapeCircle, 5.f), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVXYPadControl(nextCell(), {kFreq1, kFreq2}, "IVXYPadControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVMultiSliderControl<4>(nextCell(), "IVMultiSliderControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVMeterControl<2>(nextCell(), "IVMeterControl", style), kCtrlTagMeter, "vcontrols");
    pGraphics->AttachControl(new IVScopeControl<2>(nextCell(), "IVScopeControl", style.WithColor(kFG, COLOR_BLACK)), kCtrlTagScope, "vcontrols");
    pGraphics->AttachControl(new IVCustomControl(nextCell(), "IVCustomControl", style), kNoTag, "vcontrols");
    
    IRECT wideCell;
#ifndef OS_WEB
    wideCell = nextCell().Union(nextCell());
    pGraphics->AttachControl(new ITextControl(wideCell.GetFromTop(20.f), "File Browser (IDirBrowseControlBase) demo", style.labelText));
    pGraphics->AttachControl(new FileBrowser(wideCell.GetReducedFromTop(20.f)));
#else
    nextCell();
    nextCell();
#endif
    
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
//    pGraphics->AttachControl(new IVButtonControl(nextCell(), button2action, "Trigger open file dialog"));
//    pGraphics->AttachControl(new IVButtonControl(nextCell(), button3action, "Trigger open directory dialog"));

    wideCell = nextCell().Union(nextCell()).Union(nextCell()).Union(nextCell());
    pGraphics->AttachControl(new ITextControl(wideCell.GetFromTop(20.f), "IVKeyboardControl", style.labelText));
    pGraphics->AttachControl(new IVKeyboardControl(wideCell.GetPadded(-25), 36, 72), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IPanelControl(b.GetGridCell(4, 5, 1), COLOR_MID_GRAY));
    
    cellIdx = 31;
    
    pGraphics->AttachControl(new IVSliderControl(nextCell().GetGridCell(0, 0, 3, 1), [](IControl* pCaller) {
      pCaller->GetUI()->ForControlInGroup("vcontrols", [=](IControl& control) {
        dynamic_cast<IVectorBase&>(control).SetRoundness(pCaller->GetValue());
      });
    }, "Roundness", style, true, kHorizontal));
    
    pGraphics->AttachControl(new IVSliderControl(sameCell().GetGridCell(1, 0, 3, 1), [](IControl* pCaller) {
      pCaller->GetUI()->ForControlInGroup("vcontrols", [=](IControl& control) {
        dynamic_cast<IVectorBase&>(control).SetShadowOffset(pCaller->GetValue() * 5.f);
      });
    }, "Shadow Offset", style, true, kHorizontal));
    
    pGraphics->AttachControl(new IVSliderControl(sameCell().GetGridCell(2, 0, 3, 1), [](IControl* pCaller) {
      pCaller->GetUI()->ForControlInGroup("vcontrols", [=](IControl& control) {
        dynamic_cast<IVectorBase&>(control).SetFrameThickness(pCaller->GetValue() * 5.f);
      });
    }, "Frame Thickness", style, true, kHorizontal));
    
//    pGraphics->AttachControl(new IVSliderControl(sameCell().GetGridCell(1, 0, 3, 1), [](IControl* pCaller) {
//      dynamic_cast<IVButtonControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagVectorButton))->SetAngle(pCaller->GetValue() * 360.);
//    }, "Angle", style, true, kHorizontal));
    
    pGraphics->AttachControl(new IVToggleControl(nextCell().GetGridCell(0, 0, 3, 1), [](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      pCaller->GetUI()->ForControlInGroup("vcontrols", [=](IControl& control) {
        dynamic_cast<IVectorBase&>(control).SetDrawFrame((bool) pCaller->GetValue());
      });
    }, ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, "Draw Frame", style.WithValueText(forkAwesomeText).WithDrawFrame(false).WithDrawShadows(false), true));
    
    pGraphics->AttachControl(new IVToggleControl(sameCell().GetGridCell(1, 0, 3, 1), [](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      pCaller->GetUI()->ForControlInGroup("vcontrols", [=](IControl& control) {
        dynamic_cast<IVectorBase&>(control).SetDrawShadows((bool) pCaller->GetValue());
      });
    }, ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, "Draw Shadows", style.WithValueText(forkAwesomeText).WithDrawFrame(false).WithDrawShadows(false), true));
    
    pGraphics->AttachControl(new IVToggleControl(sameCell().GetGridCell(2, 0, 3, 1), [](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      pCaller->GetUI()->ForControlInGroup("vcontrols", [=](IControl& control) {
        dynamic_cast<IVectorBase&>(control).SetEmboss((bool) pCaller->GetValue());
      });
    }, ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, "Emboss", style.WithValueText(forkAwesomeText).WithDrawFrame(false).WithDrawShadows(false), false));
    
    wideCell = nextCell().Union(nextCell()).Union(nextCell());
    for(int colorIdx = 0; colorIdx < kNumDefaultVColors; colorIdx++)
    {
      IRECT r = wideCell.GetGridCell(colorIdx, 3, 3);
      pGraphics->AttachControl(new IVButtonControl(r, [=](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        IColor currentColor = dynamic_cast<IVButtonControl*>(pCaller)->GetColor(kFG);
        pCaller->GetUI()->PromptForColor(currentColor, "", [=](const IColor& result) {
          dynamic_cast<IVButtonControl*>(pCaller)->SetColor(kFG, result);
          pCaller->GetUI()->ForControlInGroup("vcontrols", [=](IControl& control) {
            dynamic_cast<IVectorBase&>(control).SetColor(colorIdx, result);
          });
        });
      }, kVColorStrs[colorIdx], style.WithColor(kFG, DEFAULT_SPEC.mColors[colorIdx]).WithDrawFrame(false).WithDrawShadows(false)));
    }
    
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
