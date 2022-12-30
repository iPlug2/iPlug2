#include "IPlugControls.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"
#include "IconsForkAwesome.h"
#include "IconsFontaudio.h"
#include "PageMiscControls.h"
#include "PageIVControls.h"
#include "PageISVGControls.h"
#include "PageIBControls.h"
#include "PageVisualizerControls.h"

IPlugControls::IPlugControls(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamMode)->InitEnum("Mode", 0, 4, "", IParam::kFlagsNone, "", "one", "two", "three", "four");
  GetParam(kParamFreq1)->InitDouble("Freq 1 - X", 0.5, 0.001, 10., 0.01, "Hz", IParam::kFlagsNone, "", IParam::ShapePowCurve(1.));
  GetParam(kParamFreq2)->InitDouble("Freq 2 - Y", 0.5, 0.001, 10., 0.01, "Hz", IParam::kFlagsNone, "", IParam::ShapePowCurve(1.));

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    
    if (pGraphics->NControls())
    {
      //Could handle new layout here
      return;
    }
    
//    pGraphics->EnableLiveEdit(true);
    pGraphics->EnableMouseOver(true);
    pGraphics->EnableMultiTouch(true);
    pGraphics->EnableTooltips(true);

    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, true);
    pGraphics->AttachPanelBackground(mBGControlPattern);
    pGraphics->AttachTextEntryControl();
    
#ifndef OS_IOS
    pGraphics->AttachPopupMenuControl(DEFAULT_LABEL_TEXT);
#endif
    pGraphics->AttachBubbleControl();
    
    IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("ForkAwesome", FORK_AWESOME_FN);
    pGraphics->LoadFont("Fontaudio", FONTAUDIO_FN);
    
    pGraphics->AttachControl(new IVTabbedPagesControl(b.GetPadded(-10),
    {
      {"Basic Controls", new PageMiscControls() },
      {"IVControls", new PageIVControls() },
      {"IBControls", new PageIBControls() },
      {"ISVGControls", new PageISVGControls() },
      {"VisualizerControls", new PageVisualizerControls() }
    }, ""));

////    #pragma mark IWebViewControl -
////
////    AddLabel("IWebViewControl");
////
////    auto readyFunc = [](IWebViewControl* pCaller){
////      pCaller->LoadHTML(R"(<input type="range" id="vol" name="vol" min="0" max="100" onchange='IPlugSendMsg({"msg":"SAMFUI"})'>)");
////    };
////
////    auto msgFunc = [](IWebViewControl* pCaller, const char* json){
////      auto j = json::parse(json, nullptr, false);
////      pCaller->GetUI()->GetBackgroundControl()->As<IPanelControl>()->SetPattern(IColor::GetRandomColor());
////    };
////
////    pGraphics->AttachControl(new IWebViewControl(b.GetCentredInside(200), false, readyFunc, msgFunc, "C:\\Users\\oli\\Dev\\iPlug2\\Examples\\IPlugControls\\WebView2Loader.dll", "C:\\Users\\oli\\Dev\\iPlug2\\Examples\\IPlugControls\\"));
//
////    pGraphics->AttachControl(new IVButtonControl(b.GetFromTRHC(50, 50)))->SetAnimationEndActionFunction([b](IControl* pCaller){
////      /* TODO: get webview control */->EvaluateJavaScript(R"(document.body.style.background = "#000";)");
////    });
//    
//    //pGraphics->AttachControl(new IVGroupControl("Vector Controls", "vcontrols", 10.f, 30.f, 10.f, 10.f));
//

//    
//    #pragma mark IVControl panel -
//
//    cellIdx = 31;
//    
//    nextCell();
//    
//    int slider = 0;
//    
//    pGraphics->AttachControl(new IPanelControl(b.GetGridCell(4, 5, 1), COLOR_MID_GRAY));
//    
//    for(auto label : {"Widget Frac", "Roundness", "Shadow Offset", "Frame Thickness", "Angle"})
//    {
//      pGraphics->AttachControl(new IVSliderControl(sameCell().GetGridCell(slider, 0, 5, 1), [pGraphics, slider](IControl* pCaller){
//        SplashClickActionFunc(pCaller);
//        pGraphics->ForControlInGroup("vcontrols", [pCaller, slider](IControl* pControl) {
//          
//          IVectorBase* pVControl = pControl->As<IVectorBase>();
//          float val = static_cast<float>(pCaller->GetValue());
//          
//          switch (slider) {
//            case 0 : pVControl->SetWidgetFrac(val); break;
//            case 1 : pVControl->SetRoundness(val); break;
//            case 2 : pVControl->SetShadowOffset(val * 5.f); break;
//            case 3 : pVControl->SetFrameThickness(val * 5.f); break;
//            case 4 : pVControl->SetAngle(val * 360.f); break;
//            default: break;
//          }
//        });
//      }, label, style, true, EDirection::Horizontal))->SetValue(slider == 0 ? 1.f : 0.f);
//      
//      slider++;
//    }
//    
//    nextCell();
//    
//    int toggle = 0;
//    
//    IRECT toggleRects = sameCell().FracRectHorizontal(0.49f);
//    for(auto label : {"Frame", "Shadows", "Emboss", "Show Label", "Show Value"})
//    {
//      pGraphics->AttachControl(new IVToggleControl(toggleRects.GetGridCell(toggle, 0, 5, 1), [pGraphics, toggle](IControl* pCaller){
//        SplashClickActionFunc(pCaller);
//        pGraphics->ForControlInGroup("vcontrols", [pCaller, toggle](IControl* pControl) {
//          
//          IVectorBase* pVControl = pControl->As<IVectorBase>();
//          bool val = (bool) pCaller->GetValue();
//          
//          switch (toggle) {
//            case 0 : pVControl->SetDrawFrame(val); break;
//            case 1 : pVControl->SetDrawShadows(val); break;
//            case 2 : pVControl->SetEmboss(val); break;
//            case 3 : pVControl->SetShowLabel(val); break;
//            case 4 : pVControl->SetShowValue(val); break;
//            default: break;
//          }
//        });
//      }, label, style.WithValueText(forkAwesomeText.WithSize(12.f)).WithDrawFrame(false).WithDrawShadows(false), ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, toggle == 2 ? false : true));
//      
//      toggle++;
//    }
//    
//    toggle = 0;
//    toggleRects = sameCell().FracRectHorizontal(0.49f, true);
//
//    for(auto label : {"Disable", "Show Bubble", "OS Text Entry", "OS Menu"})
//    {
//      pGraphics->AttachControl(new IVToggleControl(toggleRects.GetGridCell(toggle, 0, 5, 1), [pGraphics, toggle](IControl* pCaller){
//        SplashClickActionFunc(pCaller);
//        bool state = pCaller->GetValue() > 0.5f;
//        switch (toggle) {
//          case 0:
//            pGraphics->ForStandardControlsFunc([pCaller, toggle, state](IControl* pControl) {
//              if(pControl != pCaller)
//                pControl->SetDisabled(state);
//            });
//            break;
//          case 1:
//            pGraphics->ForStandardControlsFunc([pCaller, toggle, state](IControl* pControl) {
//              if(pControl != pCaller && pControl->GetParamIdx() == kParamGain){
//                pControl->SetActionFunction(state ? ShowBubbleHorizontalActionFunc : nullptr);
//              }});
//            break;
//          case 2:
//            if(state)
//              pGraphics->RemoveTextEntryControl();
//            else
//              pGraphics->AttachTextEntryControl();
//            break;
//          case 3:
//            if(state)
//              pGraphics->RemovePopupMenuControl();
//            else
//              pGraphics->AttachPopupMenuControl();
//            break;
//          default:
//            break;
//        }
//      }, label, style.WithValueText(forkAwesomeText.WithSize(12.f)).WithDrawFrame(false).WithDrawShadows(false), ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE));
//      
//      toggle++;
//    }
//
//    pGraphics->AttachControl(new IVRadioButtonControl(nextCell(), [pGraphics](IControl* pCaller) {
//      SplashClickActionFunc(pCaller);
//      EVShape shape = (EVShape) pCaller->As<IVRadioButtonControl>()->GetSelectedIdx();
//      pGraphics->ForControlInGroup("vcontrols", [pCaller, shape](IControl* pControl) {
//        IVectorBase* pVControl = pControl->As<IVectorBase>();
//        pVControl->SetShape(shape);
//        });
//    }, {"Rect", "Ellipse", "Triangle", "EndsRounded", "AllRounded"}, "Shape", style, EVShape::Ellipse, EDirection::Vertical, 10.f), kNoTag);
//
//    auto setColors = [pGraphics](int cell, IColor color) {
//      pGraphics->ForControlInGroup("vcontrols", [cell, color](IControl* pControl) {
//        pControl->As<IVectorBase>()->SetColor(static_cast<EVColor>(cell), color);
//        });
//    };
//
//    pGraphics->AttachControl(new IVColorSwatchControl(nextCell(), "ColorSpec", setColors, style, IVColorSwatchControl::ECellLayout::kVertical));
//
//    auto setBGColor = [pGraphics](int cell, IColor color) {
//      pGraphics->GetBackgroundControl()->As<IPanelControl>()->SetPattern(color);
//    };
//
//    pGraphics->AttachControl(new IVColorSwatchControl(nextCell().SubRectVertical(5, 0), "", setBGColor, style.WithColors({COLOR_GRAY}), IVColorSwatchControl::ECellLayout::kVertical, {kBG}, { "Background" }));
//
//    auto setLabelTextColor = [pGraphics](int cell, IColor color) {
//      pGraphics->ForControlInGroup("vcontrols", [cell, color](IControl* pControl) {
//        IVectorBase* pVControl = pControl->As<IVectorBase>();
//        IText newText = pVControl->GetStyle().labelText.WithFGColor(color);
//        pVControl->SetStyle(pVControl->GetStyle().WithLabelText(newText));
//        pControl->SetText(newText);
//        pControl->SetDirty(false);
//        });
//    };
//    
//    pGraphics->AttachControl(new IVColorSwatchControl(sameCell().SubRectVertical(5, 1), "", setLabelTextColor, style.WithColor(kBG, DEFAULT_TEXT_FGCOLOR), IVColorSwatchControl::ECellLayout::kVertical, { kBG }, { "Label Text" }));
//
//    auto setValueTextColor = [pGraphics](int cell, IColor color) {
//       pGraphics->ForControlInGroup("vcontrols", [cell, color](IControl* pControl) {
//         IVectorBase* pVControl = pControl->As<IVectorBase>();
//         IText newText = pVControl->GetStyle().valueText.WithFGColor(color);
//         pVControl->SetStyle(pVControl->GetStyle().WithValueText(newText));
////         pControl->SetText(newText);
//         pControl->SetDirty(false);
//         });
//     };
//
//    pGraphics->AttachControl(new IVColorSwatchControl(sameCell().SubRectVertical(5, 2), "", setValueTextColor, style.WithColor(kBG, DEFAULT_TEXT_FGCOLOR), IVColorSwatchControl::ECellLayout::kVertical, { kBG }, { "Value Text" }));
//    
//    auto setLabelTextSize = [pGraphics](IControl* pCaller) {
//      float newSize = (float) pCaller->As<IVNumberBoxControl>()->GetRealValue();
//      pGraphics->ForControlInGroup("vcontrols", [newSize](IControl* pControl) {
//        IVectorBase* pVControl = pControl->As<IVectorBase>();
//        IText newText = pVControl->GetStyle().labelText.WithSize(newSize);
//        pVControl->SetStyle(pVControl->GetStyle().WithLabelText(newText));
//        pControl->OnResize();
//        pControl->SetDirty(false);
//      });
//    };
//    
//    pGraphics->AttachControl(new IVNumberBoxControl(sameCell().SubRectVertical(5, 3).FracRectHorizontal(0.5f), kNoParameter, setLabelTextSize, "Label", style, false, (double) style.labelText.mSize, 12., 100.));
//    
//    auto setValueTextSize = [pGraphics](IControl* pCaller) {
//      float newSize = (float) pCaller->As<IVNumberBoxControl>()->GetRealValue();
//      pGraphics->ForControlInGroup("vcontrols", [newSize](IControl* pControl) {
//        IVectorBase* pVControl = pControl->As<IVectorBase>();
//        IText newText = pVControl->GetStyle().valueText.WithSize(newSize);
//        pVControl->SetStyle(pVControl->GetStyle().WithValueText(newText));
//        pControl->SetText(newText);
//        pControl->OnResize();
//        pControl->SetDirty(false);
//      });
//    };
//    
//    pGraphics->AttachControl(new IVNumberBoxControl(sameCell().SubRectVertical(5, 3).FracRectHorizontal(0.5f, true), kNoParameter, setValueTextSize, "Value", style, false, (double) style.valueText.mSize, 12., 100.));
//
//    auto promptLabelFont = [pGraphics](IControl* pCaller) {
//      auto completionHandler = [pGraphics](const WDL_String& fileName, const WDL_String& path) {
//        if (fileName.GetLength())
//        {
//          if (pGraphics->LoadFont(fileName.get_filepart(), fileName.Get()))
//          {
//            pGraphics->ForControlInGroup("vcontrols", [fileName](IControl* pControl) {
//              IVectorBase* pVControl = pControl->As<IVectorBase>();
//              IText newText = pVControl->GetStyle().labelText.WithFont(fileName.get_filepart());
//              pVControl->SetStyle(pVControl->GetStyle().WithLabelText(newText));
//              pControl->OnResize();
//              pControl->SetDirty(false);
//            });
//          }
//        }
//      };
//      
//      WDL_String fileName, path;
//      pGraphics->PromptForFile(fileName, path, EFileAction::Open, "ttf", completionHandler);
//    };
//    
//    pGraphics->AttachControl(new IVButtonControl(sameCell().SubRectVertical(5, 4).FracRectHorizontal(0.5f), SplashClickActionFunc, "font...", style.WithDrawShadows(false)))->SetAnimationEndActionFunction(promptLabelFont);
//    
//    auto promptValueFont = [pGraphics](IControl* pCaller) {
//      auto completionHandler = [pGraphics](const WDL_String& fileName, const WDL_String& path){
//        if (fileName.GetLength())
//        {
//          if (pGraphics->LoadFont(fileName.get_filepart(), fileName.Get()))
//          {
//            pGraphics->ForControlInGroup("vcontrols", [fileName](IControl* pControl) {
//              IVectorBase* pVControl = pControl->As<IVectorBase>();
//              IText newText = pVControl->GetStyle().valueText.WithFont(fileName.get_filepart());
//              pVControl->SetStyle(pVControl->GetStyle().WithValueText(newText));
//              pControl->OnResize();
//              pControl->SetText(newText);
//              pControl->SetDirty(false);
//            });
//          }
//        }
//      };
//      
//      WDL_String fileName, path;
//      pGraphics->PromptForFile(fileName, path, EFileAction::Open, "ttf", completionHandler);
//    };
//    
//    pGraphics->AttachControl(new IVButtonControl(sameCell().SubRectVertical(5, 4).FracRectHorizontal(0.5f, true), SplashClickActionFunc, "font...", style.WithDrawShadows(false)))->SetAnimationEndActionFunction(promptValueFont);
  };
#endif
}

#if IPLUG_EDITOR
void IPlugControls::FlashBlueLED()
{
  GetUI()->GetControlWithTag(kCtrlTagBlueLED)->As<ILEDControl>()->TriggerWithDecay(1000);
}

void IPlugControls::OnMidiMsgUI(const IMidiMsg& msg)
{
  if(GetUI())
  {
    switch (msg.StatusMsg()) {
      case iplug::IMidiMsg::kNoteOn:
        FlashBlueLED();
        break;
      default:
        break;
    }
  }
}

void IPlugControls::OnUIClose()
{
  // store the background pattern. No modifications to other controls are stored, and this would also need to be serialized in plugin state, for recall!
  mBGControlPattern = GetUI()->GetBackgroundControl()->As<IPanelControl>()->GetPattern();
}

#endif

#if IPLUG_DSP
void IPlugControls::OnIdle()
{
  mScopeSender.TransmitData(*this);
  mMeterSender.TransmitData(*this);
  mRTTextSender.TransmitData(*this);
  mDisplaySender.TransmitData(*this);
  mPeakAvgMeterSender.TransmitData(*this);

  float val = std::fabs(mLastOutputData.vals[0]);
  SendControlValueFromDelegate(kCtrlTagRedLED, std::copysign(val, mLastOutputData.vals[0]));
//  SendControlValueFromDelegate(kCtrlTagGreenLED, std::copysign(val, -mLastOutputData.vals[0]));
}

void IPlugControls::OnReset()
{
  mPeakAvgMeterSender.Reset(GetSampleRate());
}

void IPlugControls::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double phaseIncr1 = (1. / GetSampleRate()) * GetParam(kParamFreq1)->Value();
  const double phaseIncr2 = (1. / GetSampleRate()) * GetParam(kParamFreq2)->Value();

  for (int s = 0; s < nFrames; s++) {
    static double phase1 = 0.;
    static double phase2 = 0.;

    outputs[0][s] = cos(phase1 += phaseIncr1);
    outputs[1][s] = sin(phase2 += phaseIncr2);
  }
  
  mDisplaySender.ProcessBlock(outputs, nFrames, kCtrlTagDisplay);
  mScopeSender.ProcessBlock(outputs, nFrames, kCtrlTagScope);
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
  mPeakAvgMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagPeakAvgMeter);

  mLastOutputData.vals[0] = (float) outputs[0][0]; // just take first value in block

  mRTTextSender.PushData(mLastOutputData);

  for (int s = 0; s < nFrames; s++) {
    outputs[0][s] = 0.;
    outputs[1][s] = 0.;
  }
}
#endif
