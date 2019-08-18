/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestSizeControl
 */

#include "IControl.h"

/** Control to display the size of a region
 *   @ingroup TestControls */
class TestSizeControl : public IControl
{
public:
  TestSizeControl(const IRECT& bounds)
  : IControl(bounds)
  {
    mIgnoreMouse = true;
    mText = IText(15, COLOR_BLACK, nullptr, EAlign::Near);
  }

  void Draw(IGraphics& g) override
  {
    g.DrawLine(COLOR_BLACK, 10, 0, 10, mRECT.B);
    g.DrawLine(COLOR_BLACK, 0, mRECT.B-10, mRECT.R, mRECT.B-10);
    g.DrawText(mText, mWidthStr.Get(), mRECT.L + 15, mRECT.T + 5);
    g.DrawText(mText, mHeightStr.Get(), mRECT.R - 40, mRECT.B - 20);
    g.DrawText(mText, mScaleStr.Get(), mRECT.L + 15, mRECT.B - 20);
  }

  void OnRescale() override
  {
  }

  void OnResize() override
  {
    mWidthStr.SetFormatted(32, "%i px", (int) mRECT.W());
    mHeightStr.SetFormatted(32, "%i px", (int) mRECT.H());
    mScaleStr.SetFormatted(64, "width %i, height %i, scale %0.2f", GetUI()->WindowWidth(), GetUI()->WindowHeight(),  GetUI()->GetDrawScale());
  }

private:
  WDL_String mWidthStr;
  WDL_String mHeightStr;
  WDL_String mScaleStr;
};
