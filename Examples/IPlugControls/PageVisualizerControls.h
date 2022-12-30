#pragma once

#include "IVTabbedPagesControl.h"

using namespace iplug;
using namespace igraphics;

class PageVisualizerControls : public IVTabbedPageBase
{
public:
  PageVisualizerControls()
  : IVTabbedPageBase()
  {
  }
  
  void OnAttached() override
  {
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
    //    AddLabel("ILEDControl");
    AddChildControl(new ILEDControl(IRECT(), COLOR_RED), kCtrlTagRedLED);
    AddChildControl(new IVMeterControl<2>(IRECT(), "IVMeterControl - Lin", style.WithColor(kFG, COLOR_WHITE.WithOpacity(0.3f)), EDirection::Vertical, {"L", "R"}), kCtrlTagMeter, "vcontrols");
    AddChildControl(new IVPeakAvgMeterControl<2>(IRECT(), "IVPeakAvgMeterControl - Log", style.WithColor(kFG, COLOR_WHITE.WithOpacity(0.3f))), kCtrlTagPeakAvgMeter, "vcontrols");
    AddChildControl(new IVScopeControl<2, kScopeBufferSize*2>(IRECT(), "IVScopeControl", style.WithColor(kFG, COLOR_BLACK)), kCtrlTagScope, "vcontrols");
    AddChildControl(new IVDisplayControl(IRECT(), "IVDisplayControl", style, EDirection::Vertical, -1., 1., 0., 512), kCtrlTagDisplay, "vcontrols");
  }
  
  void OnResize() override
  {
    ForAllChildrenFunc([this](int childIdx, IControl* pControl) {
      pControl->SetTargetAndDrawRECTs(this->mRECT.GetPadded(-10.0f).GetGridCell(childIdx, 4, 5).GetPadded(-10.0f));
    });
  }
};
