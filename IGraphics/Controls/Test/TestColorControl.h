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
  TestColorControl(IGEditorDelegate& dlg, IRECT rect)
  :  IControl(dlg, rect)
  {
    SetTooltip("TestColorControl");
  }
  
  void OnResize() override
  {
    int idx = 0;
    float nstops = 7.;
    float pos = 0.;
    auto nextStop = [&]() {
      pos = (1.f/nstops) * idx++;
      return IColorStop{IColor::GetFromHSLA(pos, 1., 0.5), pos};
    };
    
    mPattern = IPattern::CreateLinearGradient(mRECT.L, mRECT.MH(), mRECT.R, mRECT.MH(),
    {
      nextStop(),
      nextStop(),
      nextStop(),
      nextStop(),
      nextStop(),
      nextStop(),
      nextStop(),
      nextStop(),
    });
  }

  void Draw(IGraphics& g) override
  {
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
  IPattern mPattern = IPattern(kLinearPattern);
};
