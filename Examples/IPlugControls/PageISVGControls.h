#pragma once

#include "IVTabbedPagesControl.h"

using namespace iplug;
using namespace igraphics;

class PageISVGControls : public IVTabbedPageBase
{
public:
  PageISVGControls()
  : IVTabbedPageBase()
  {
  }
  
  void OnAttached() override
  {
    const ISVG sliderHandleSVG = GetUI()->LoadSVG(SVGSLIDERHANDLE_FN);
    const ISVG sliderTrackSVG = GetUI()->LoadSVG(SVGSLIDERTRACK_FN);
    const ISVG hsliderHandleSVG = GetUI()->LoadSVG(SVGHSLIDERHANDLE_FN);
    const ISVG hsliderTrackSVG = GetUI()->LoadSVG(SVGHSLIDERTRACK_FN);
    const ISVG knobSVG = GetUI()->LoadSVG(SVGKNOBROTATE_FN);
   
   //    AddLabel("ISVGSliderControl");
    AddChildControl(new ISVGSliderControl(IRECT(), sliderHandleSVG, sliderTrackSVG, kParamGain, EDirection::Vertical), kNoTag, "svgcontrols");
    AddChildControl(new ISVGSliderControl(IRECT(), hsliderHandleSVG, hsliderTrackSVG, kParamGain, EDirection::Horizontal), kNoTag, "svgcontrols")->SetTooltip("ISVGSlider H");

//    AddLabel("ISVGKnobControl");
    AddChildControl(new ISVGKnobControl(IRECT(), knobSVG, kParamGain), kNoTag, "svgcontrols");
  }
  
  void OnResize() override
  {
    ForAllChildrenFunc([this](int childIdx, IControl* pControl) {
      pControl->SetTargetAndDrawRECTs(this->mRECT.GetPadded(-10.0f).GetGridCell(childIdx, 4, 5).GetPadded(-10.0f));
    });
  }
};
