/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestFontControl
 */

#include "IControl.h"

/** Control to test drawing fonts
 *   @ingroup TestControls */
class TestFontControl : public IControl
{
  static const int size = 36;
    
public:
    TestFontControl(const IRECT& bounds)
  : IControl(bounds), mCount(-1), mFontCount(0), mStrCount(0)
  {
    SetTooltip("TestFontControl");
    mDblAsSingleClick = true;
    Next();
  }

  void Draw(IGraphics& g) override
  {
    int pos = mCount / 3;
    IRECT rect = mRECT;
    
    if (pos == 0)
      rect = mRECT.GetFromTop(size);
    else if (pos == 1)
      rect = mRECT.GetCentredInside(mRECT.W(), size);
    else
      rect = mRECT.GetFromBottom(size);
      
    const char* str = mStrCount ? "Quickly dog" : "Font Test";
      
    g.FillRect(COLOR_WHITE, mRECT);
    g.FillRect(COLOR_MID_GRAY, rect);
    g.DrawText(mText, str, mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    Next();
    SetDirty(false);
  }

  void Next()
  {
    if (++mCount > 8)
    {
      mCount = 0;
      mFontCount = 1 - mFontCount;
    }
    
    mStrCount = 1 - mStrCount;
      
    IColor c = DEFAULT_TEXT_FGCOLOR;
    const char* font = mFontCount ? "Roboto-Regular" : "Alternative Font";
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

  int mCount;
  int mFontCount;
  int mStrCount;
};
