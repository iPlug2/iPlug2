/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IVColorPickerControl.h"
#include "IControls.h"

using namespace iplug;
using namespace igraphics;

IVColorPickerControl::IVColorPickerControl(const IVStyle& style)
: IContainerBase(IRECT())
, IVectorBase(style)
{
  AttachIControl(this, "");
}

void IVColorPickerControl::OnAttached()
{
  auto layout = GetLayout(mWidgetBounds);

  AddChildControl(new IVXYPadControl(layout["xypad"], {kNoParameter, kNoParameter}, "", mStyle, 10.f, false, false));

  auto hslFunc = [this](IControl* pCaller){
    this->mColor = IColor::FromHSLA(1.f-(float) pCaller->GetValue(), 1.f, 0.5f, 1.f);
    SetDirty();
  };
  
  AddChildControl(new IVSliderControl(layout["hslSlider"], hslFunc, "", DEFAULT_STYLE.WithColor(kSH, COLOR_TRANSPARENT).WithColor(kX1, COLOR_TRANSPARENT).WithDrawFrame(true).WithAngle(90), false, EDirection::Vertical, DEFAULT_GEARING, 10.f, 12.f, false, -10.f, 1.f))->As<IVSliderControl>()->SetShape(EVShape::Triangle);
}

void IVColorPickerControl::OnResize()
{
  mPanel = mRECT.GetPadded(-10.f);
  MakeRects(mPanel, false);
  
  auto layout = GetLayout(mWidgetBounds);
  
  if (NChildren())
  {
    GetChild(0)->SetTargetAndDrawRECTs(layout["xypad"]);
    GetChild(1)->SetTargetAndDrawRECTs(layout["hslSlider"]);
  }
}

void IVColorPickerControl::Draw(IGraphics& g)
{
  g.DrawFastDropShadow(mPanel, mRECT, 5.f, 5.f);
  g.FillRoundRect(COLOR_LIGHT_GRAY, mPanel, 5.f);
  
  auto layout = GetLayout(mWidgetBounds);

  g.PathRect(layout["xypad"]);

  g.PathFill(IPattern::CreateLinearGradient(layout["xypad"], EDirection::Horizontal, {{COLOR_WHITE, 0.f}, {mColor, 1.f}}), {true});
  g.PathFill(IPattern::CreateLinearGradient(layout["xypad"], EDirection::Vertical, {{{0, 0, 0, 0}, 0.f}, {COLOR_BLACK, 1.f}}));
  
  const int nSegs = 10;
  auto hslSlider = layout["hslSlider"];
  for (int i=0; i<nSegs; i++) {
    auto r = hslSlider.SubRectVertical(nSegs, i).GetPadded(0, 0, 0, 1);
    g.PathRect(r);
    const float seg = 1.f/(float)nSegs;
    g.PathFill(IPattern::CreateLinearGradient(r, EDirection::Vertical, {{IColor::FromHSLA(i*seg, 1.f, 0.55f), 0.f},
                                                                        {IColor::FromHSLA((i+1)*seg, 1.f, 0.55f), 1.f}}));
  }
}

bool IVColorPickerControl::CreateColorPicker(float x, float y, IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  SetTargetRECT(GetUI()->GetBounds());
  SetRECT(IRECT::MakeXYWH(x, y, 300, 300));
  SetLabelStr(str);
  
  return false;
}

void IVColorPickerControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(!mRECT.Contains(x, y))
  {
    SetTargetAndDrawRECTs(IRECT());
    GetUI()->SetAllControlsDirty();
    return;
  }
}

//static
const ILayoutMap IVColorPickerControl::GetLayout(const IRECT& bounds)
{
  auto inner = bounds.GetPadded(-10.f);
  auto top = inner.FracRectVertical(0.5f, true);
  auto xypad = top.FracRectHorizontal(0.75f);
  auto hslSlider = top.FracRectHorizontal(0.25f, true).GetMidHPadded(20.f);
  
  return
  {
    {"inner", inner},
    {"xypad", xypad},
    {"hslSlider", hslSlider}
  };
}
