#include "IPlugControls.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"
#include "IconsForkAwesome.h"
#include "IconsFontaudio.h"

IPlugControls::IPlugControls(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamMode)->InitEnum("Mode", 0, 4, "", IParam::kFlagsNone, "", "one", "two", "three", "four");
  GetParam(kParamFreq1)->InitDouble("Freq 1 - X", 0.5, 0., 2, 0.01, "Hz");
  GetParam(kParamFreq2)->InitDouble("Freq 2 - Y", 0.5, 0., 2, 0.01, "Hz");

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
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->EnableTooltips(true);
    pGraphics->AttachTextEntryControl();
    pGraphics->AttachPopupMenuControl(DEFAULT_LABEL_TEXT);
    
    IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("ForkAwesome", FORK_AWESOME_FN);
    pGraphics->LoadFont("Fontaudio", FONTAUDIO_FN);

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
        COLOR_BLACK, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(12.f, EAlign::Center) // Label text
    };
    
    const IText forkAwesomeText {24.f, "ForkAwesome"};
    const IText bigLabel {24, COLOR_WHITE, "Roboto-Regular", EAlign::Near, EVAlign::Top, 0};
    const IText fontaudioText {32.f, "Fontaudio"};

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
    pGraphics->AttachControl(new ICaptionControl(sameCell().FracRectVertical(0.5, true).GetMidVPadded(10.f), kParamGain, IText(24.f), DEFAULT_FGCOLOR, false));
    pGraphics->AttachControl(new ICaptionControl(sameCell().FracRectVertical(0.5, false).GetMidVPadded(10.f), kParamMode, IText(24.f), DEFAULT_FGCOLOR, false));

    AddLabel("IBKnobControl");
    pGraphics->AttachControl(new IBKnobControl(sameCell().GetPadded(-5.), bitmap1, kParamGain));
    AddLabel("IBKnobRotaterControl");
    pGraphics->AttachControl(new IBKnobRotaterControl(sameCell().GetPadded(-5.), bitmap2, kParamGain));
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
    pGraphics->AttachControl(new ISVGKnob(sameCell().GetCentredInside(100), knobSVG, kParamGain));

    auto button1action = [pGraphics](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      pGraphics->ShowMessageBox("Message Title", "Message", kMB_YESNO, [&](EMsgBoxResult result) {
                                                      WDL_String str;
                                                      str.SetFormatted(32, "%s pressed", kMessageResultStrs[result]);
                                                      dynamic_cast<ITextControl*>(pGraphics->GetControlWithTag(kCtrlTagDialogResult))->SetStr(str.Get());
                                                    });
    };

    pGraphics->AttachControl(new IVKnobControl(nextCell().GetCentredInside(110.), kParamGain, "IVKnobControl", style, true), kNoTag, "vcontrols");
//    pGraphics->AttachControl(new IVKnobSwitchControl(nextCell().GetCentredInside(110.), kParamMode, "IVKnobSwitchControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVSliderControl(nextCell().GetCentredInside(110.), kParamGain, "IVSliderControl", style, true), kCtrlTagVectorSlider, "vcontrols");
    pGraphics->AttachControl(new IVRangeSliderControl(nextCell().GetCentredInside(110.), {kParamFreq1, kParamFreq2}, "IVRangeSliderControl", style, EDirection::Horizontal, true, 8.f, 2.f), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVButtonControl(nextCell().GetCentredInside(110.), button1action, "IVButtonControl", style, false), kCtrlTagVectorButton, "vcontrols");
    AddLabel("IVButtonControl 2");
    pGraphics->AttachControl(new IVButtonControl(sameCell().GetCentredInside(110.), button1action, "Label in button", style, true), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVButtonControl(nextCell().GetCentredInside(110.), [pGraphics](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      static IPopupMenu menu {"Menu", {"one", "two", "three"}, [pCaller](int indexInMenu, IPopupMenu::Item* itemChosen) {
          if(itemChosen)
            dynamic_cast<IVButtonControl*>(pCaller)->SetValueStr(itemChosen->GetText());
        }
      };
      
      float x, y;
      pGraphics->GetMouseDownPoint(x, y);
      pGraphics->CreatePopupMenu(*pCaller, menu, x, y);
      
    }, "IVButtonControl 3", style.WithValueText(IText(36.f, EVAlign::Middle)),  false, true), kNoTag, "vcontrols");
    dynamic_cast<IVButtonControl*>(pGraphics->GetControl(pGraphics->NControls()-1))->SetValueStr("one");
    
    pGraphics->AttachControl(new IVSwitchControl(nextCell().GetCentredInside(110.), kParamMode, "IVSwitchControl", style.WithValueText(IText(36.f, EAlign::Center))), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVToggleControl(nextCell().GetCentredInside(110.), SplashClickActionFunc, "IVToggleControl", style.WithValueText(forkAwesomeText), "", ICON_FK_CHECK), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVRadioButtonControl(nextCell().GetCentredInside(110.), kParamMode, "IVRadioButtonControl", style, EVShape::Ellipse, EDirection::Vertical, 10.f), kCtrlTagRadioButton, "vcontrols");
    pGraphics->AttachControl(new IVTabSwitchControl(nextCell().GetFromTop(50.), SplashClickActionFunc, {ICON_FAU_FILTER_LOWPASS, ICON_FAU_FILTER_BANDPASS, ICON_FAU_FILTER_HIGHPASS}, "IVTabSwitchControl", style.WithValueText(fontaudioText), EVShape::EndsRounded), kCtrlTagTabSwitch, "vcontrols");
    pGraphics->AttachControl(new IVXYPadControl(nextCell(), {kParamFreq1, kParamFreq2}, "IVXYPadControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVMultiSliderControl<4>(nextCell(), "IVMultiSliderControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVMeterControl<2>(nextCell(), "IVMeterControl", style), kCtrlTagMeter, "vcontrols");
    pGraphics->AttachControl(new IVScopeControl<2>(nextCell(), "IVScopeControl", style.WithColor(kFG, COLOR_BLACK)), kCtrlTagScope, "vcontrols");
//    pGraphics->AttachControl(new IVCustomControl(nextCell(), "IVCustomControl", style), kNoTag, "vcontrols");
    
    IRECT wideCell;
#ifndef OS_WEB
    wideCell = nextCell().Union(nextCell());
    pGraphics->AttachControl(new ITextControl(wideCell.GetFromTop(20.f), "File Browser (IDirBrowseControlBase) demo", style.labelText));
    pGraphics->AttachControl(new FileBrowser(wideCell.GetReducedFromTop(20.f)));
#else
    nextCell();
    nextCell();
#endif
    wideCell = nextCell().Union(nextCell()).Union(nextCell()).Union(nextCell());
    pGraphics->AttachControl(new ITextControl(wideCell.GetFromTop(20.f), "IVKeyboardControl", style.labelText));
    pGraphics->AttachControl(new IVKeyboardControl(wideCell.GetPadded(-25), 36, 72), kNoTag);
    pGraphics->AttachControl(new IVLabelControl(nextCell(), "Test", DEFAULT_STYLE.WithLabelText(DEFAULT_LABEL_TEXT.WithSize(50.f).WithFGColor(COLOR_WHITE))), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVSlideSwitchControl(nextCell(), kParamMode, "IVSlideSwitchControl", style, true), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVPlotControl(nextCell(), {{COLOR_RED,  [](double x){ return std::sin(x * 6.2);} },
                                                            {COLOR_BLUE, [](double x){ return std::cos(x * 6.2);} },
                                                            {COLOR_GREEN, [](double x){ return x > 0.5;} }

                                                            }, 32, "IVPlotControl", style.WithFrameThickness(3.f)), kNoTag, "vcontrols");
    
#pragma mark -
    cellIdx = 31;
    
    nextCell();
    
    int slider = 0;
    
    pGraphics->AttachControl(new IPanelControl(b.GetGridCell(4, 5, 1), COLOR_MID_GRAY));
    
    for(auto label : {"Widget Frac", "Roundness", "Shadow Offset", "Frame Thickness", "Angle"})
    {
      pGraphics->AttachControl(new IVSliderControl(sameCell().GetGridCell(slider, 0, 5, 1), [pGraphics, slider](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        pGraphics->ForControlInGroup("vcontrols", [pCaller, slider](IControl& control) {
          
          IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
          float val = static_cast<float>(pCaller->GetValue());
          
          switch (slider) {
            case 0 : vcontrol.SetWidgetFrac(val); break;
            case 1 : vcontrol.SetRoundness(val); break;
            case 2 : vcontrol.SetShadowOffset(val * 5.f); break;
            case 3 : vcontrol.SetFrameThickness(val * 5.f); break;
            case 4 : vcontrol.SetAngle(val * 360.f); break;
            default: break;
          }
        });
      }, label, style, true, EDirection::Horizontal));
      
      slider++;
    }
    
    nextCell();
    
    int toggle = 0;
    
    for(auto label : {"Draw Frame", "Draw Shadows", "Show Label", "Show Value"})
    {
      pGraphics->AttachControl(new IVToggleControl(sameCell().GetGridCell(toggle, 0, 5, 1), [pGraphics, toggle](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        pGraphics->ForControlInGroup("vcontrols", [pCaller, toggle](IControl& control) {
          
          IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
          bool val = (bool) pCaller->GetValue();
          
          switch (toggle) {
            case 0 : vcontrol.SetDrawFrame(val); break;
            case 1 : vcontrol.SetDrawShadows(val); break;
            case 2 : vcontrol.SetShowLabel(val); break;
            case 3 : vcontrol.SetShowValue(val); break;
            default: break;
          }
        });
      }, label, style.WithValueText(forkAwesomeText.WithSize(12.f)).WithDrawFrame(false).WithDrawShadows(false), ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, true));
      
      toggle++;
    }

    pGraphics->AttachControl(new IVRadioButtonControl(nextCell(), [pGraphics](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      EVShape shape = (EVShape) dynamic_cast<IVRadioButtonControl*>(pCaller)->GetSelectedIdx();
      dynamic_cast<IVButtonControl*>(pGraphics->GetControlWithTag(kCtrlTagVectorButton))->SetShape(shape);
      dynamic_cast<IVTabSwitchControl*>(pGraphics->GetControlWithTag(kCtrlTagTabSwitch))->SetShape(shape);
      dynamic_cast<IVSliderControl*>(pGraphics->GetControlWithTag(kCtrlTagVectorSlider))->SetShape(shape);
      dynamic_cast<IVRadioButtonControl*>(pGraphics->GetControlWithTag(kCtrlTagRadioButton))->SetShape(shape);

    }, {"Rect", "Ellipse", "Triangle", "EndsRounded", "AllRounded"}, "Shape", style, EVShape::Ellipse, EDirection::Vertical, 10.f), kNoTag);
    
    wideCell = nextCell().Union(nextCell()).Union(nextCell());
    for(int colorIdx = 0; colorIdx < kNumDefaultVColors; colorIdx++)
    {
      IRECT r = wideCell.GetGridCell(colorIdx, 3, 3);
      pGraphics->AttachControl(new IVButtonControl(r, [pGraphics, colorIdx](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        IColor currentColor = dynamic_cast<IVButtonControl*>(pCaller)->GetColor(kFG);
        pGraphics->PromptForColor(currentColor, "", [pCaller, pGraphics, colorIdx](const IColor& result) {
          dynamic_cast<IVButtonControl*>(pCaller)->SetColor(kFG, result);
          pGraphics->ForControlInGroup("vcontrols", [pCaller, colorIdx, result](IControl& control) {
            dynamic_cast<IVectorBase&>(control).SetColor(colorIdx, result);
          });
        });
      }, kVColorStrs[colorIdx], style.WithColor(kFG, DEFAULT_COLOR_SPEC.mColors[colorIdx]).WithDrawFrame(false).WithDrawShadows(false)));
    }
    
    pGraphics->AttachControl(new IVButtonControl(nextCell(), [pGraphics](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      
      IPanelControl* pPanel = dynamic_cast<IPanelControl*>(pGraphics->GetBackgroundControl());
      IColor color = pPanel->GetPattern().GetStop(0).mColor;
      pGraphics->PromptForColor(color, "", [pCaller, pGraphics, pPanel](const IColor& result){
        dynamic_cast<IVButtonControl*>(pCaller)->SetColor(kFG, result);
        pPanel->SetPattern(result);
      });

    }, "Background", style.WithColor(kFG, DEFAULT_GRAPHICS_BGCOLOR).WithDrawFrame(false).WithDrawShadows(false)));

    nextCell();
    toggle = 0;
    
    for(auto label : {"Disable"})
    {
      pGraphics->AttachControl(new IVToggleControl(sameCell().GetGridCell(toggle, 0, 5, 1), [pGraphics, toggle](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        bool disable = pCaller->GetValue() > 0.5f;
        pGraphics->ForStandardControlsFunc([pCaller, toggle, disable](IControl& control) {
          
          switch (toggle) {
            case 0 :
              if(&control != pCaller)
                control.SetDisabled(disable); break;
            default:
              break;
          }
        });
      }, label, style.WithValueText(forkAwesomeText.WithSize(12.f)).WithDrawFrame(false).WithDrawShadows(false), ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE));
      
      toggle++;
    }
    
  };
#endif
}

#if IPLUG_DSP
void IPlugControls::OnIdle()
{
  mScopeSender.TransmitData(*this);
  mMeterSender.TransmitData(*this);
}

void IPlugControls::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double phaseIncr1 = GetParam(kParamFreq1)->Value() * 0.00001;
  const double phaseIncr2 = GetParam(kParamFreq2)->Value() * 0.00001;

  for (int s = 0; s < nFrames; s++) {
    static double phase1 = 0.;
    static double phase2 = 0.;

    outputs[0][s] = cos(phase1 += phaseIncr1);
    outputs[1][s] = sin(phase2 += phaseIncr2);
  }
  
  mScopeSender.ProcessBlock(outputs, nFrames);
  mMeterSender.ProcessBlock(outputs, nFrames);

  for (int s = 0; s < nFrames; s++) {
    outputs[0][s] = 0.;
    outputs[1][s] = 0.;
  }
}
#endif
