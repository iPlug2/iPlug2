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

/** Control to test drawing text
 *   @ingroup TestControls */
class TestFontControl : public IControl
{
  static const int size = 20;
    
public:
    TestFontControl(IRECT bounds)
  : IControl(bounds), mCount(-1), mFontCount(0)
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
      
    g.FillRect(COLOR_WHITE, mRECT);
    g.FillRect(COLOR_MID_GRAY, rect);
    g.DrawText(mText, "Quickly dog", mRECT);
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
    
    IColor c = DEFAULT_TEXT_FGCOLOR;
    const char* font = mFontCount ? "Roboto-Regular" : "Times New Roman";
    if (mCount == 0)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignNear, IText::kVAlignTop);
    else if (mCount == 1)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignTop);
    else if (mCount == 2)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignFar, IText::kVAlignTop);
    else if (mCount == 3)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignNear, IText::kVAlignMiddle);
    else if (mCount == 4)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignMiddle);
    else if (mCount == 5)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignFar, IText::kVAlignMiddle);
    else if (mCount == 6)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignNear, IText::kVAlignBottom);
    else if (mCount == 7)
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom);
    else
      mText = IText(size, c, font, IText::kStyleNormal, IText::kAlignFar, IText::kVAlignBottom);
  }

private:

  int mCount;
  int mFontCount;
  int mStringIndex;
};
