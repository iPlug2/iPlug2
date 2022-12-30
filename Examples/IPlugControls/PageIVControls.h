#pragma once

#include "IVTabbedPagesControl.h"

using namespace iplug;
using namespace igraphics;

static constexpr int kScopeBufferSize = 128;

class PageIVControls : public IVTabbedPageBase
{
public:
  PageIVControls()
  : IVTabbedPageBase()
  {
  }
  
  void OnAttached() override
  {
   const IText forkAwesomeText {16.f, "ForkAwesome"};
   const IText bigLabel {24, COLOR_WHITE, "Roboto-Regular", EAlign::Near, EVAlign::Top, 0};
   const IText fontaudioText {32.f, "Fontaudio"};

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

    AddChildControl(new IVKnobControl(IRECT(), kParamGain, "IVKnobControl", style, true), kNoTag, "vcontrols");
    AddChildControl(new IVKnobControlWithMarks(IRECT(), kParamGain, "IVKnobControlWithMarks", style.WithWidgetFrac(0.5f), true), kNoTag, "vcontrols");

    AddChildControl(new IVSliderControl(IRECT(), kParamGain, "IVSliderControl", style.WithRoundness(1.f), true, EDirection::Vertical, DEFAULT_GEARING, 6.f, 6.f, true), kCtrlTagVectorSliderV, "vcontrols");
    AddChildControl(new IVSliderControlWithMarks(IRECT(), kParamGain, "IVSliderControlWithMarks", style.WithRoundness(1.f), true, EDirection::Vertical, DEFAULT_GEARING, 6.f, 6.f, true), kNoTag, "vcontrols");
    AddChildControl(new IVSliderControl(IRECT(), kParamGain, "IVSliderControl Horizontal", style, true, EDirection::Horizontal), kCtrlTagVectorSliderH, "vcontrols");
    AddChildControl(new IVRangeSliderControl(IRECT(), {kParamFreq1, kParamFreq2}, "IVRangeSliderControl", style, EDirection::Horizontal, true, 8.f, 2.f), kNoTag, "vcontrols");
    
    auto button1action = [](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      pCaller->GetUI()->ShowMessageBox("Message Title", "Message", kMB_YESNO,
        [pCaller](EMsgBoxResult result) {
        WDL_String str;
        str.SetFormatted(32, "%s pressed", kMessageResultStrs[result]);
        pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult)->As<ITextControl>()->SetStr(str.Get());
      });
    };
    
    AddChildControl(new IVButtonControl(IRECT(), button1action, "IVButtonControl", style, false), kCtrlTagVectorButton, "vcontrols");
//    AddChildControl(new IVButtonControl(IRECT(), button1action, "IVButtonControl: Label in button", style, true), kNoTag, "vcontrols");

//    AddChildControl(new IVButtonControl(IRECT(), [](IControl* pCaller){
//      SplashClickActionFunc(pCaller);
//      static IPopupMenu menu {"Menu", {"one", "two", "three"}, [pCaller](IPopupMenu* pMenu) {
//          auto* itemChosen = pMenu->GetChosenItem();
//          if (itemChosen)
//            pCaller->As<IVButtonControl>()->SetValueStr(itemChosen->GetText());
//        }
//      };
//
//      float x, y;
//      pCaller->GetUI()->GetMouseDownPoint(x, y);
//      pCaller->GetUI()->CreatePopupMenu(*pCaller, menu, x, y);
//
//    }, "IVButtonControl: pop up a menu", style.WithValueText(IText(24.f, EVAlign::Middle)), false, true), kNoTag, "vcontrols")
//    ->As<IVButtonControl>()->SetValueStr("one");
    
    AddChildControl(new IVSwitchControl(IRECT(), kParamMode, "IVSwitchControl", style.WithValueText(IText(24.f, EAlign::Center))), kNoTag, "vcontrols");
    AddChildControl(new IVToggleControl(IRECT(), SplashClickActionFunc, "IVToggleControl", style.WithValueText(forkAwesomeText), "", ICON_FK_CHECK), kNoTag, "vcontrols");
    AddChildControl(new IVMenuButtonControl(IRECT(), kParamMode, "IVMenuButtonControl", style), kNoTag, "vcontrols");
    AddChildControl(new IVRadioButtonControl(IRECT(), kParamMode, {"one", "two", "three", "four"}, "IVRadioButtonControl", style, EVShape::Ellipse, EDirection::Vertical, 10.f), kCtrlTagRadioButton, "vcontrols");
    AddChildControl(new IVTabSwitchControl(IRECT(), SplashClickActionFunc, {ICON_FAU_FILTER_LOWPASS, ICON_FAU_FILTER_BANDPASS, ICON_FAU_FILTER_HIGHPASS}, "IVTabSwitchControl", style.WithValueText(fontaudioText), EVShape::EndsRounded), kCtrlTagTabSwitch, "vcontrols");
    AddChildControl(new IVSlideSwitchControl(IRECT(), kParamMode, "IVSlideSwitchControl", style, true), kNoTag, "vcontrols");
    AddChildControl(new IVXYPadControl(IRECT(), {kParamFreq1, kParamFreq2}, "IVXYPadControl", style), kNoTag, "vcontrols");
    AddChildControl(new IVMultiSliderControl<4>(IRECT(), "IVMultiSliderControl", style), kNoTag, "vcontrols");

    AddChildControl(new IVLabelControl(IRECT(), "IVLabelControl"), kNoTag, "vcontrols");
    AddChildControl(new IVColorSwatchControl(IRECT(), "IVColorSwatchControl", [](int, IColor){}, style, IVColorSwatchControl::ECellLayout::kHorizontal, {kX1, kX2, kX3}, {"", "", ""}), kNoTag, "vcontrols");
    AddChildControl(new IVNumberBoxControl(IRECT(), kParamGain, nullptr, "IVNumberBoxControl", style, true, 50.f, 1.f, 100.f, "%0.0f", false), kNoTag, "vcontrols");
    AddChildControl(new IVPlotControl(IRECT(), {{COLOR_RED,  [](double x){ return std::sin(x * 6.2);} },
                                                            {COLOR_BLUE, [](double x){ return std::cos(x * 6.2);} },
                                                            {COLOR_GREEN, [](double x){ return x > 0.5;} }

                                                            }, 32, "IVPlotControl", style), kNoTag, "vcontrols");

    // IRECT wideCell;
    // wideCell = Union(IRECT()).Union(IRECT()).Union(IRECT());
    // AddChildControl(new ITextControl(wideCell.GetFromTop(20.f), "IVKeyboardControl", style.labelText));
     AddChildControl(new IWheelControl(IRECT()));
//     AddChildControl(new IVKeyboardControl(IRECT(), kNoTag))->SetActionFunction([this](IControl* pControl){
    //   this->FlashBlueLED();
//     });
  }
  
  void OnResize() override
  {
    ForAllChildrenFunc([this](int childIdx, IControl* pControl) {
      pControl->SetTargetAndDrawRECTs(this->mRECT.GetPadded(-10.0f).GetGridCell(childIdx, 4, 5).GetPadded(-10.0f));
    });
  }
};
