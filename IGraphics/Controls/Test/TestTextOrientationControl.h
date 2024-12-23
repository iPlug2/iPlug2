/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestTextOrientationControl
 */

#include "IControl.h"

/** Control to test drawing text with orientation
 *   @ingroup TestControls */
class TestTextOrientationControl : public IKnobControlBase
{
  static const int size = 36;
    
public:
    TestTextOrientationControl(const IRECT& bounds, int paramIdx)
  : IKnobControlBase(bounds, paramIdx)
  {
    SetTooltip("TestTextOrientationControl");
    mDblAsSingleClick = true;
    Next(true);
    SetValue(0.5);
  }

  void Draw(IGraphics& g) override
  {
    IRECT drawRECT = mRECT;
    const char* str = "Some Text To Rotate";
    mText.mAngle = static_cast<float>(GetValue()) * 360.f - 180.f;
    
    g.PathClipRegion(mRECT);
    g.MeasureText(mText, str, drawRECT);
    g.FillRect(COLOR_WHITE, mRECT);
    g.FillRect(COLOR_MID_GRAY, drawRECT);
    g.DrawText(mText, str, mRECT);
    g.PathClipRegion();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mDrag = false;
  }
    
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (!mDrag)
    {
      Next();
      SetDirty(false);
    }
  }
    
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    mDrag = true;
    IKnobControlBase::OnMouseDrag(x, y, dX, dY, mod);
  }

  void Next(bool reset = false)
  {
    int align = static_cast<int>(mText.mAlign);
    int vAlign = static_cast<int>(mText.mVAlign);
    
    if (reset || ++align > static_cast<int>(EAlign::Far))
    {
      align = static_cast<int>(EAlign::Near);
      
      if (reset || ++vAlign > static_cast<int>(EVAlign::Baseline))
        vAlign = static_cast<int>(EVAlign::Top);
    }
      
    IColor c = DEFAULT_TEXT_FGCOLOR;
    const char* font = "Roboto-Regular";
    mText = IText(size, c, font, static_cast<EAlign>(align), static_cast<EVAlign>(vAlign));
  }

private:
  bool mDrag = false;
};
