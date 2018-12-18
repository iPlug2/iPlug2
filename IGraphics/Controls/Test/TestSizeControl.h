/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"

class TestSizeControl : public IControl
{
public:
  TestSizeControl(IGEditorDelegate& dlg, IRECT bounds)
  : IControl(dlg, bounds)
  {
    mIgnoreMouse = true;
    mText = IText(15, COLOR_BLACK, nullptr, IText::kStyleNormal, IText::kAlignNear);
  }

  void Draw(IGraphics& g) override
  {
    g.DrawHorizontalLine(COLOR_BLACK, mRECT, 0.05);
    g.DrawVerticalLine(COLOR_BLACK, mRECT, 0.05);
    g.DrawText(mText, mWidthStr.Get(), mRECT.L + 5, mRECT.T + 5);
    g.DrawText(mText, mHeightStr.Get(), mRECT.R - 40, mRECT.B - 20);
    g.DrawText(mText, mScaleStr.Get(), mRECT.MW(), mRECT.B - 20);
  }

  void OnRescale() override
  {
    
  }
  
  void OnResize() override
  {
    mWidthStr.SetFormatted(32, "%i px", (int) mRECT.W());
    mHeightStr.SetFormatted(32, "%i px", (int) mRECT.H());
    mScaleStr.SetFormatted(50, "Window width %i, height %i, scale %0.2f", GetUI()->WindowWidth(), GetUI()->WindowHeight(), GetUI()->GetDrawScale());
  }
  
private:
  WDL_String mWidthStr;
  WDL_String mHeightStr;
  WDL_String mScaleStr;
};
