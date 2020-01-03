/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestTextSizeControl
 */

#include "IControl.h"

/** Control to test drawing text with varying size
 *   @ingroup TestControls */
class TestTextSizeControl : public IKnobControlBase
{
  static const int size = 14;
    
public:
    TestTextSizeControl(const IRECT& bounds, int paramIdx)
  : IKnobControlBase(bounds, paramIdx)
  , mCount(-1)
  {
    SetTooltip("TestTextSizeControl");
    mDblAsSingleClick = true;
    Next();
    SetValue(0.2);
  }

  void Draw(IGraphics& g) override
  {
    const char* str = "Some Text To Resize";
    mText.mSize = static_cast<float>(GetValue()) * 40.f + 5.f;
    
    g.FillRect(COLOR_WHITE, mRECT);
    g.DrawText(mText, str, mRECT);
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

  void Next()
  {
    if (++mCount > 8)
      mCount = 0;
      
    IColor c = DEFAULT_TEXT_FGCOLOR;
    const char* font = "Roboto-Regular";
    if (mCount == 0)
      mText = IText(size, c, font, EAlign::Near, EVAlign::Top);
    else if (mCount == 1)
      mText = IText(size, c, font, EAlign::Center, EVAlign::Top);
    else if (mCount == 2)
      mText = IText(size, c, font, EAlign::Far, EVAlign::Top);
    else if (mCount == 3)
      mText = IText(size, c, font, EAlign::Near, EVAlign::Middle);
    else if (mCount == 4)
      mText = IText(size, c, font, EAlign::Center, EVAlign::Middle);
    else if (mCount == 5)
      mText = IText(size, c, font, EAlign::Far, EVAlign::Middle);
    else if (mCount == 6)
      mText = IText(size, c, font, EAlign::Near, EVAlign::Bottom);
    else if (mCount == 7)
      mText = IText(size, c, font, EAlign::Center, EVAlign::Bottom);
    else
      mText = IText(size, c, font, EAlign::Far, EVAlign::Bottom);
  }

private:
  bool mDrag = false;
  int mCount;
};
