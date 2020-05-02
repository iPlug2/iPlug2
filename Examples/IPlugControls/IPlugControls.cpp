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

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
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
    pGraphics->EnableMouseOver(true);
    pGraphics->EnableMultiTouch(true);
    pGraphics->EnableTooltips(true);

    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachTextEntryControl();
    pGraphics->AttachPopupMenuControl(DEFAULT_LABEL_TEXT);
    pGraphics->AttachBubbleControl();
    
    IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("ForkAwesome", FORK_AWESOME_FN);
    pGraphics->LoadFont("Fontaudio", FONTAUDIO_FN);

    const IBitmap knobBitmap = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
    const IBitmap knobRotateBitmap = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);
    const IBitmap switchBitmap = pGraphics->LoadBitmap((PNGSWITCH_FN), 2, true);
    const IBitmap buttonBitmap = pGraphics->LoadBitmap(PNGBUTTON_FN, 10);
    const IBitmap sliderHandleBitmap = pGraphics->LoadBitmap(PNGSLIDERHANDLE_FN);
    const IBitmap sliderTrackBitmap = pGraphics->LoadBitmap(PNGSLIDERTRACK_FN);
    const IBitmap bitmapText = pGraphics->LoadBitmap(PNGTEXT_FN, 95, true);
    const ISVG sliderHandleSVG = pGraphics->LoadSVG(SVGSLIDERHANDLE_FN);
    const ISVG sliderTrackSVG = pGraphics->LoadSVG(SVGSLIDERTRACK_FN);

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
    
    const IText forkAwesomeText {20.f, "ForkAwesome"};
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
  
    
#pragma mark MiscControls -
    
    AddLabel("ITextControl");
    pGraphics->AttachControl(new ITextControl(sameCell().SubRectVertical(4, 1).GetMidVPadded(10.f), "Result...", DEFAULT_TEXT, COLOR_LIGHT_GRAY), kCtrlTagDialogResult, "misccontrols");
    
    pGraphics->AttachControl(new IURLControl(sameCell().SubRectVertical(4, 2).GetMidVPadded(10.f), "IURLControl", "https://iplug2.github.io", DEFAULT_TEXT), kNoTag, "misccontrols");

    pGraphics->AttachControl(new IEditableTextControl(sameCell().SubRectVertical(4, 3).GetMidVPadded(10.f), "IEditableTextControl", DEFAULT_TEXT), kNoTag, "misccontrols");
    
    AddLabel("ITextToggleControl");
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 0, 3, 3), nullptr, ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, forkAwesomeText), kNoTag, "misccontrols");
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 1, 3, 3), nullptr, ICON_FK_CIRCLE_O, ICON_FK_CHECK_CIRCLE, forkAwesomeText), kNoTag, "misccontrols");
    pGraphics->AttachControl(new ITextToggleControl(sameCell().GetGridCell(1, 2, 3, 3), nullptr, ICON_FK_PLUS_SQUARE, ICON_FK_MINUS_SQUARE, forkAwesomeText), kNoTag, "misccontrols");

    AddLabel("ICaptionControl");
    pGraphics->AttachControl(new ICaptionControl(sameCell().SubRectVertical(4, 1).GetMidVPadded(10.f), kParamGain, IText(24.f), DEFAULT_FGCOLOR, false), kNoTag, "misccontrols");
    pGraphics->AttachControl(new ICaptionControl(sameCell().SubRectVertical(4, 2).GetMidVPadded(10.f), kParamMode, IText(24.f), DEFAULT_FGCOLOR, false), kNoTag, "misccontrols");
    
    //pGraphics->AttachControl(new IVGroupControl("Misc Controls", "misccontrols", 5.f, 35.f, 10.f, 15.f));

#pragma mark IBControls -
    
    AddLabel("IBKnobControl");
    pGraphics->AttachControl(new IBKnobControl(sameCell().GetPadded(-5.), knobBitmap, kParamGain), kNoTag, "bcontrols");
    AddLabel("IBKnobRotaterControl");
    pGraphics->AttachControl(new IBKnobRotaterControl(sameCell().GetPadded(-5.), knobRotateBitmap, kParamGain), kNoTag, "bcontrols");
    AddLabel("IBButtonControl");
    pGraphics->AttachControl(new IBButtonControl(sameCell().FracRectVertical(0.6f, true), buttonBitmap, [](IControl* pCaller) {
      pCaller->SetAnimation([](IControl* pCaller){
        auto progress = pCaller->GetAnimationProgress();
        if(progress > 1.) {
          pCaller->OnEndAnimation();
          return;
        }
        pCaller->SetValue(Clip(progress + .5, 0., 1.));
      }, 100);
    }), kNoTag, "bcontrols");
//    AddLabel("IBSwitchControl");
    pGraphics->AttachControl(new ITextControl(sameCell().FracRectVertical(0.5f, false).GetFromTop(20.f), "IBSwitchControl", style.labelText));
    pGraphics->AttachControl(new IBSwitchControl(sameCell().FracRectVertical(0.5f, false), switchBitmap), kNoTag, "bcontrols");
    AddLabel("IBSliderControl");
    pGraphics->AttachControl(new IBSliderControl(sameCell().GetCentredInside(sliderHandleBitmap.W(), 100.f), sliderHandleBitmap, sliderTrackBitmap, kParamGain, EDirection::Vertical), kNoTag, "bcontrols");
    //pGraphics->AttachControl(new IVGroupControl("Bitmap Controls", "bcontrols", 10.f, 30.f, 30.f, 10.f));
    AddLabel("IBTextControl");
    pGraphics->AttachControl(new IBTextControl(sameCell(), bitmapText, DEFAULT_LABEL_TEXT, "HELLO", 10, 16, 0, false));
#pragma mark ISVGControls -
    
    AddLabel("ISVGKnobControl");
    pGraphics->AttachControl(new ISVGKnobControl(sameCell().GetCentredInside(100), knobSVG, kParamGain), kNoTag, "svgcontrols");
    
    AddLabel("ISVGSliderControl");
    pGraphics->AttachControl(new ISVGSliderControl(sameCell().GetCentredInside(30, 100), sliderHandleSVG, sliderTrackSVG, kParamGain), kNoTag, "svgcontrols");
    //pGraphics->AttachControl(new IVGroupControl("SVG Controls", "svgcontrols", 10.f, 30.f, 10.f, 10.f));

#pragma mark IVControls -

    pGraphics->AttachControl(new IVKnobControl(nextCell().GetCentredInside(110.), kParamGain, "IVKnobControl", style, true), kNoTag, "vcontrols");
    
    pGraphics->AttachControl(new IVSliderControl(nextCell(), kParamGain, "IVSliderControl", style.WithRoundness(1.f), true, EDirection::Vertical, DEFAULT_GEARING, 6.f, 6.f, true), kCtrlTagVectorSlider, "vcontrols");
    pGraphics->AttachControl(new IVSliderControl(nextCell().SubRectVertical(3, 0), kParamGain, "IVSliderControl H", style, true, EDirection::Horizontal), kCtrlTagVectorSlider, "vcontrols");
    pGraphics->AttachControl(new IVRangeSliderControl(sameCell().SubRectVertical(3, 1), {kParamFreq1, kParamFreq2}, "IVRangeSliderControl", style, EDirection::Horizontal, true, 8.f, 2.f), kNoTag, "vcontrols");

    auto button1action = [pGraphics](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      pGraphics->ShowMessageBox("Message Title", "Message", kMB_YESNO, [&](EMsgBoxResult result) {
                                                      WDL_String str;
                                                      str.SetFormatted(32, "%s pressed", kMessageResultStrs[result]);
                                                      dynamic_cast<ITextControl*>(pGraphics->GetControlWithTag(kCtrlTagDialogResult))->SetStr(str.Get());
                                                    });
    };
    
    pGraphics->AttachControl(new IVButtonControl(nextCell().SubRectVertical(3, 0), button1action, "IVButtonControl", style, false), kCtrlTagVectorButton, "vcontrols");
    pGraphics->AttachControl(new IVButtonControl(sameCell().SubRectVertical(3, 1), button1action, "Label in button", style, true), kNoTag, "vcontrols");

    pGraphics->AttachControl(new IVButtonControl(sameCell().SubRectVertical(3, 2), [pGraphics](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      static IPopupMenu menu {"Menu", {"one", "two", "three"}, [pCaller](IPopupMenu* pMenu) {
          auto* itemChosen = pMenu->GetChosenItem();
          if(itemChosen)
            pCaller->As<IVButtonControl>()->SetValueStr(itemChosen->GetText());
        }
      };
      
      float x, y;
      pGraphics->GetMouseDownPoint(x, y);
      pGraphics->CreatePopupMenu(*pCaller, menu, x, y);
      
    }, "", style.WithValueText(IText(24.f, EVAlign::Middle)), false, true), kNoTag, "vcontrols");
    pGraphics->GetControl(pGraphics->NControls()-1)->As<IVButtonControl>()->SetValueStr("one");
    
    pGraphics->AttachControl(new IVSwitchControl(nextCell().SubRectVertical(3, 0), kParamMode, "IVSwitchControl", style.WithValueText(IText(24.f, EAlign::Center))), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVToggleControl(sameCell().SubRectVertical(3, 1), SplashClickActionFunc, "IVToggleControl", style.WithValueText(forkAwesomeText), "", ICON_FK_CHECK), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVRadioButtonControl(nextCell().GetCentredInside(110.), kParamMode, {}, "IVRadioButtonControl", style, EVShape::Ellipse, EDirection::Vertical, 10.f), kCtrlTagRadioButton, "vcontrols");
    pGraphics->AttachControl(new IVTabSwitchControl(nextCell().SubRectVertical(3, 0), SplashClickActionFunc, {ICON_FAU_FILTER_LOWPASS, ICON_FAU_FILTER_BANDPASS, ICON_FAU_FILTER_HIGHPASS}, "IVTabSwitchControl", style.WithValueText(fontaudioText), EVShape::EndsRounded), kCtrlTagTabSwitch, "vcontrols");
    pGraphics->AttachControl(new IVSlideSwitchControl(sameCell().SubRectVertical(3, 1), kParamMode, "IVSlideSwitchControl", style, true), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVXYPadControl(nextCell(), {kParamFreq1, kParamFreq2}, "IVXYPadControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVMultiSliderControl<4>(nextCell(), "IVMultiSliderControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVMeterControl<2>(nextCell(), "IVMeterControl", style), kCtrlTagMeter, "vcontrols");
    pGraphics->AttachControl(new IVScopeControl<2>(nextCell(), "IVScopeControl", style.WithColor(kFG, COLOR_BLACK)), kCtrlTagScope, "vcontrols");
    
    pGraphics->AttachControl(new IVLabelControl(nextCell().SubRectVertical(3, 0).GetMidVPadded(10.f), "IVLabelControl", DEFAULT_STYLE.WithValueText(DEFAULT_VALUE_TEXT.WithSize(20.f).WithFGColor(COLOR_WHITE).WithVAlign(EVAlign::Middle))), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVColorSwatchControl(sameCell().SubRectVertical(3, 1), "IVColorSwatchControl", [](int, IColor){}, style, IVColorSwatchControl::ECellLayout::kHorizontal, {kX1, kX2, kX3}, {"", "", ""}), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVNumberBoxControl(sameCell().SubRectVertical(3, 2), kParamGain, nullptr, "IVNumberBoxControl", style), kNoTag, "vcontrols");
    pGraphics->AttachControl(new IVPlotControl(nextCell(), {{COLOR_RED,  [](double x){ return std::sin(x * 6.2);} },
                                                            {COLOR_BLUE, [](double x){ return std::cos(x * 6.2);} },
                                                            {COLOR_GREEN, [](double x){ return x > 0.5;} }

                                                            }, 32, "IVPlotControl", style), kNoTag, "vcontrols");
    
    IRECT wideCell;
    nextCell();
    wideCell = nextCell().Union(nextCell()).Union(nextCell()).Union(nextCell());
    pGraphics->AttachControl(new ITextControl(wideCell.GetFromTop(20.f), "IVKeyboardControl", style.labelText));
    pGraphics->AttachControl(new IVKeyboardControl(wideCell.GetPadded(-25), 36, 72), kNoTag);

    //pGraphics->AttachControl(new IVGroupControl("Vector Controls", "vcontrols", 10.f, 30.f, 10.f, 10.f));

    #pragma mark IVControl panel -

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
      }, label, style, true, EDirection::Horizontal))->SetValue(slider == 0 ? 1.f : 0.f);
      
      slider++;
    }
    
    nextCell();
    
    int toggle = 0;
    
    IRECT toggleRects = sameCell().FracRectHorizontal(0.49f);
    for(auto label : {"Frame", "Shadows", "Emboss", "Show Label", "Show Value"})
    {
      pGraphics->AttachControl(new IVToggleControl(toggleRects.GetGridCell(toggle, 0, 5, 1), [pGraphics, toggle](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        pGraphics->ForControlInGroup("vcontrols", [pCaller, toggle](IControl& control) {
          
          IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
          bool val = (bool) pCaller->GetValue();
          
          switch (toggle) {
            case 0 : vcontrol.SetDrawFrame(val); break;
            case 1 : vcontrol.SetDrawShadows(val); break;
            case 2 : vcontrol.SetEmboss(val); break;
            case 3 : vcontrol.SetShowLabel(val); break;
            case 4 : vcontrol.SetShowValue(val); break;
            default: break;
          }
        });
      }, label, style.WithValueText(forkAwesomeText.WithSize(12.f)).WithDrawFrame(false).WithDrawShadows(false), ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, toggle == 2 ? false : true));
      
      toggle++;
    }
    
    toggle = 0;
    toggleRects = sameCell().FracRectHorizontal(0.49f, true);

    for(auto label : {"Disable", "Show Bubble"})
    {
      pGraphics->AttachControl(new IVToggleControl(toggleRects.GetGridCell(toggle, 0, 5, 1), [pGraphics, toggle](IControl* pCaller){
        SplashClickActionFunc(pCaller);
        bool state = pCaller->GetValue() > 0.5f;
        pGraphics->ForStandardControlsFunc([pCaller, toggle, state](IControl& control) {
          
          switch (toggle) {
            case 0 :
              if(&control != pCaller)
                control.SetDisabled(state); break;
            case 1 :
              if(&control != pCaller)
              {
                if(control.GetParamIdx() == kParamGain)
                  control.SetActionFunction(state ? ShowBubbleHorizontalActionFunc : nullptr); break;
              }
            default:
              break;
          }
        });
      }, label, style.WithValueText(forkAwesomeText.WithSize(12.f)).WithDrawFrame(false).WithDrawShadows(false), ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE));
      
      toggle++;
    }

    pGraphics->AttachControl(new IVRadioButtonControl(nextCell(), [pGraphics](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      EVShape shape = (EVShape) pCaller->As<IVRadioButtonControl>()->GetSelectedIdx();
      pGraphics->ForControlInGroup("vcontrols", [pCaller, shape](IControl& control) {
        IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
        vcontrol.SetShape(shape);
        });
    }, {"Rect", "Ellipse", "Triangle", "EndsRounded", "AllRounded"}, "Shape", style, EVShape::Ellipse, EDirection::Vertical, 10.f), kNoTag);

    auto setColors = [pGraphics](int cell, IColor color) {
      pGraphics->ForControlInGroup("vcontrols", [cell, color](IControl& control) {
        dynamic_cast<IVectorBase&>(control).SetColor(static_cast<EVColor>(cell), color);
        });
    };

    pGraphics->AttachControl(new IVColorSwatchControl(nextCell(), "ColorSpec", setColors, style, IVColorSwatchControl::ECellLayout::kVertical));

    auto setBGColor = [pGraphics](int cell, IColor color) {
      pGraphics->GetBackgroundControl()->As<IPanelControl>()->SetPattern(color);
    };

    pGraphics->AttachControl(new IVColorSwatchControl(nextCell().SubRectVertical(5, 0), "", setBGColor, style.WithColors({COLOR_GRAY}), IVColorSwatchControl::ECellLayout::kVertical, {kBG}, { "Background" }));

    auto setLabelTextColor = [pGraphics](int cell, IColor color) {
      pGraphics->ForControlInGroup("vcontrols", [cell, color](IControl& control) {
        IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
        IText newText = vcontrol.GetStyle().labelText.WithFGColor(color);
        vcontrol.SetStyle(vcontrol.GetStyle().WithLabelText(newText));
        control.SetText(newText);
        control.SetDirty(false);
        });
    };
    
    pGraphics->AttachControl(new IVColorSwatchControl(sameCell().SubRectVertical(5, 1), "", setLabelTextColor, style.WithColor(kBG, DEFAULT_TEXT_FGCOLOR), IVColorSwatchControl::ECellLayout::kVertical, { kBG }, { "Label Text" }));

    auto setValueTextColor = [pGraphics](int cell, IColor color) {
       pGraphics->ForControlInGroup("vcontrols", [cell, color](IControl& control) {
         IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
         IText newText = vcontrol.GetStyle().valueText.WithFGColor(color);
         vcontrol.SetStyle(vcontrol.GetStyle().WithValueText(newText));
//         control.SetText(newText);
         control.SetDirty(false);
         });
     };

    pGraphics->AttachControl(new IVColorSwatchControl(sameCell().SubRectVertical(5, 2), "", setValueTextColor, style.WithColor(kBG, DEFAULT_TEXT_FGCOLOR), IVColorSwatchControl::ECellLayout::kVertical, { kBG }, { "Value Text" }));
    
    auto setLabelTextSize = [pGraphics](IControl* pCaller) {
      float newSize = (float) pCaller->As<IVNumberBoxControl>()->GetRealValue();
      pGraphics->ForControlInGroup("vcontrols", [newSize](IControl& control) {
        IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
        IText newText = vcontrol.GetStyle().labelText.WithSize(newSize);
        vcontrol.SetStyle(vcontrol.GetStyle().WithLabelText(newText));
        control.OnResize();
        control.SetDirty(false);
      });
    };
    
    pGraphics->AttachControl(new IVNumberBoxControl(sameCell().SubRectVertical(5, 3), kNoParameter, setLabelTextSize, "Label Text Size", style, (double) style.labelText.mSize, 12., 100.));
    
    auto setValueTextSize = [pGraphics](IControl* pCaller) {
      float newSize = (float) pCaller->As<IVNumberBoxControl>()->GetRealValue();
      pGraphics->ForControlInGroup("vcontrols", [newSize](IControl& control) {
        IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
        IText newText = vcontrol.GetStyle().valueText.WithSize(newSize);
        vcontrol.SetStyle(vcontrol.GetStyle().WithValueText(newText));
        control.SetText(newText);
        control.OnResize();
        control.SetDirty(false);
      });
    };
    
    pGraphics->AttachControl(new IVNumberBoxControl(sameCell().SubRectVertical(5, 4), kNoParameter, setValueTextSize, "Value Text Size", style, (double) style.valueText.mSize, 12., 100.));

    auto promptLabelFont = [pGraphics](IControl* pCaller) {
      WDL_String fileName;
      WDL_String path;
      pGraphics->PromptForFile(fileName, path, EFileAction::Open, "ttf");
      
      if(fileName.GetLength())
      {
        if(pGraphics->LoadFont(fileName.get_filepart(), fileName.Get()))
        {
          pGraphics->ForControlInGroup("vcontrols", [fileName](IControl& control) {
            IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
            IText newText = vcontrol.GetStyle().labelText.WithFont(fileName.get_filepart());
            vcontrol.SetStyle(vcontrol.GetStyle().WithLabelText(newText));
            control.OnResize();
            control.SetDirty(false);
          });
        }
      }
    };
    
    pGraphics->AttachControl(new IVButtonControl(nextCell().SubRectVertical(3, 0).FracRectHorizontal(0.5), SplashClickActionFunc, "Label Font...", style))->SetAnimationEndActionFunction(promptLabelFont);
    
    auto promptValueFont = [pGraphics](IControl* pCaller) {
      WDL_String fileName;
      WDL_String path;
      pGraphics->PromptForFile(fileName, path, EFileAction::Open, "ttf");
      
      if(fileName.GetLength())
      {
        if(pGraphics->LoadFont(fileName.get_filepart(), fileName.Get()))
        {
          pGraphics->ForControlInGroup("vcontrols", [fileName](IControl& control) {
            IVectorBase& vcontrol = dynamic_cast<IVectorBase&>(control);
            IText newText = vcontrol.GetStyle().valueText.WithFont(fileName.get_filepart());
            vcontrol.SetStyle(vcontrol.GetStyle().WithValueText(newText));
            control.OnResize();
            control.SetText(newText);
            control.SetDirty(false);
          });
        }
      }
    };
    
    pGraphics->AttachControl(new IVButtonControl(sameCell().SubRectVertical(3, 0).FracRectHorizontal(0.5, true), SplashClickActionFunc, "Value Font...", style))->SetAnimationEndActionFunction(promptValueFont);
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
  
  mScopeSender.ProcessBlock(outputs, nFrames, kCtrlTagScope);
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);

  for (int s = 0; s < nFrames; s++) {
    outputs[0][s] = 0.;
    outputs[1][s] = 0.;
  }
}
#endif
