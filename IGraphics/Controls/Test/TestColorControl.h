/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestColorControl
 */

#include "IControl.h"

/** Control to colors
 *   @ingroup TestControls */
class TestColorControl : public IControl
{
public:
  TestColorControl(const IRECT& rect)
  : IControl(rect)
  {
    SetTooltip("TestColorControl");
  }
  
  void OnResize() override
  {
    mPattern = IPattern::CreateLinearGradient(mRECT.L, mRECT.MH(), mRECT.R, mRECT.MH());
    const int nstops = 7;
    
    for (int i=0; i<nstops; i++) {
      float pos = (1.f/(float) nstops) * i;
      mPattern.AddStop(IColor::GetFromHSLA(pos, 1., 0.5), pos);
    }
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);

#ifndef IGRAPHICS_NANOVG
    if(g.HasPathSupport())
    {
      g.PathRect(mRECT);
      g.PathFill(mPattern);
    }
#else
    g.DrawText(mText, "UNSUPPORTED", mRECT);
#endif
  }

private:
  IPattern mPattern = IPattern(EPatternType::Linear);
};
