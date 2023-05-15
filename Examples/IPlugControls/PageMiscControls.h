#pragma once

#include "IVTabbedPagesControl.h"

using namespace iplug;
using namespace igraphics;

class PageMiscControls : public IVTabbedPageBase
{
public:
  PageMiscControls()
  : IVTabbedPageBase()
  {
  }
  
  void OnAttached() override
  {
    const IText forkAwesomeText {16.f, "ForkAwesome"};

    //AddLabel("ITextControl");
    AddChildControl(new ITextControl(IRECT(), "Result...", DEFAULT_TEXT, COLOR_LIGHT_GRAY), kCtrlTagDialogResult, "misccontrols");
    AddChildControl(new IURLControl(IRECT(), "IURLControl", "https://iplug2.github.io", DEFAULT_TEXT), kNoTag, "misccontrols");
    AddChildControl(new IEditableTextControl(IRECT(), "IEditableTextControl", DEFAULT_TEXT), kNoTag, "misccontrols");
    
    //AddLabel("ITextToggleControl");
    AddChildControl(new ITextToggleControl(IRECT(), nullptr, ICON_FK_SQUARE_O, ICON_FK_CHECK_SQUARE, forkAwesomeText), kNoTag, "misccontrols");
    AddChildControl(new ITextToggleControl(IRECT(), nullptr, ICON_FK_CIRCLE_O, ICON_FK_CHECK_CIRCLE, forkAwesomeText), kNoTag, "misccontrols");
    AddChildControl(new ITextToggleControl(IRECT(), nullptr, ICON_FK_PLUS_SQUARE, ICON_FK_MINUS_SQUARE, forkAwesomeText), kNoTag, "misccontrols");

    AddChildControl(new IRTTextControl<1, float>(IRECT(), "IRTTextControl: %0.2f", ":", "IRTTextControl"), kCtrlTagRTText);

    //AddLabel("ICaptionControl");
    AddChildControl(new ICaptionControl(IRECT(), kParamGain, IText(24.f), DEFAULT_FGCOLOR, false), kNoTag, "misccontrols");
    AddChildControl(new ICaptionControl(IRECT(), kParamMode, IText(24.f), DEFAULT_FGCOLOR, false), kNoTag, "misccontrols");

    //    AddLabel("ILambdaControl");
    AddChildControl(new ILambdaControl(IRECT(),
    [](ILambdaControl* pCaller, IGraphics& g, IRECT& r) {
      const float radius = r.W();
      const float x = r.MW();
      const float y = r.MH();
      const float rotate = float(pCaller->GetAnimationProgress() * PI);
      
      for(int index = 0, limit = 40; index < limit; ++index)
      {
        float firstAngle = float ((index * 2 * PI) / limit);
        float secondAngle = float (((index + 1) * 2 * PI) / limit);
        
        g.PathTriangle(x, y,
                        x + std::sin(firstAngle + rotate) * radius, y + std::cos(firstAngle + rotate) * radius,
                        x + std::sin(secondAngle + rotate) * radius, y + std::cos(secondAngle + rotate) * radius);
        
        if(index % 2)
          g.PathFill(COLOR_RED);
        else
          g.PathFill(pCaller->mMouseInfo.ms.L ? COLOR_VIOLET : COLOR_BLUE);
      }
      
    }, 1000, false));
  }
  
  void OnResize() override
  {
    ForAllChildrenFunc([this](int childIdx, IControl* pControl) {
      pControl->SetTargetAndDrawRECTs(this->mRECT.GetPadded(-10.0f).GetGridCell(childIdx, 4, 5).GetPadded(-10.0f));
    });
  }
};
