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
  : IControl(bounds), mFontCount(1), mStrCount(0)
  {
    SetTooltip("TestFontControl");
    mDblAsSingleClick = true;
    Next(true);
  }

  void Draw(IGraphics& g) override
  {
    IRECT rect = mRECT;
      
    const char* str = mStrCount ? "Quickly dog" : "Font Test";
    
    g.PathClipRegion(mRECT);
    g.FillRect(COLOR_WHITE, mRECT);
    g.MeasureText(mText, str, rect);
    rect.L = mRECT.L;
    rect.R = mRECT.R;
    g.FillRect(COLOR_MID_GRAY, rect);
    g.DrawText(mText, str, mRECT);
    g.PathClipRegion();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    Next();
    SetDirty(false);
  }

  void Next(bool reset = false)
  {
    int align = static_cast<int>(mText.mAlign);
    int vAlign = static_cast<int>(mText.mVAlign);
    
    if (reset || ++align > static_cast<int>(EAlign::Far))
    {
      align = static_cast<int>(EAlign::Near);
      
      if (reset || ++vAlign > static_cast<int>(EVAlign::Baseline))
      {
        vAlign = static_cast<int>(EVAlign::Top);
        mFontCount = 1 - mFontCount;
      }
    }
    
    mStrCount = 1 - mStrCount;
      
    IColor c = DEFAULT_TEXT_FGCOLOR;
    const char* font = mFontCount ? "Roboto-Regular" : "Alternative Font";
    
    mText = IText(size, c, font, static_cast<EAlign>(align), static_cast<EVAlign>(vAlign));
  }

private:

  int mFontCount;
  int mStrCount;
};
