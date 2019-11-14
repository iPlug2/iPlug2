/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestBezierControl
 */

#include "IControl.h"

/** Control to test drawing bezier curves
 *   @ingroup TestControls */
class TestBezierControl : public IControl
{
public:
  TestBezierControl(const IRECT& rect)
  : IControl(rect)
  {
    SetTooltip("TestBezierControl");
    mP1 = mRECT.GetFromTLHC(20, 20);
    mP2 = mRECT.GetFromTRHC(20, 20);
    mC1 = mRECT.GetFromBLHC(20, 20);
    mC2 = mRECT.GetFromBRHC(20, 20);
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_WHITE, mRECT);
    

    g.FillRect(COLOR_BLACK, mP1.GetCentredInside(5.f));
    g.FillRect(COLOR_BLACK, mP2.GetCentredInside(5.f));
    g.FillRect(COLOR_RED, mC1.GetCentredInside(5.f));
    g.FillRect(COLOR_RED, mC2.GetCentredInside(5.f));
    
    g.PathMoveTo(mP1.MW(), mP1.MH());
    g.PathCubicBezierTo(mC1.MW(), mC1.MH(), mC2.MW(), mC2.MH(), mP2.MW(), mP2.MH());

    g.PathStroke(COLOR_BLUE, 2.f);

    g.PathMoveTo(mP1.MW(), mP1.MH());
    g.PathQuadraticBezierTo(mC1.MW(), mC1.MH(), mP2.MW(), mP2.MH());
    g.PathStroke(COLOR_GREEN, 2.f);

    
    if(mMouseOverRect)
      g.DrawRect(COLOR_BLACK, mMouseOverRect->GetCentredInside(8.f));
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    OnMouseDrag(x, y, 0., 0., mod);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mMouseOverRect = nullptr;
  }
  
  void OnMouseDrag(float x, float y, float, float, const IMouseMod& mod) override
  {
    if(mMouseOverRect)
      *mMouseOverRect = IRECT(x-2.5f, y-2.5f, x+2.5f, y+2.5f);
    SetDirty(false);
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    if(mP1.Contains(x, y))
      mMouseOverRect = &mP1;
    else if(mP2.Contains(x, y))
      mMouseOverRect = &mP2;
    else if(mC1.Contains(x, y))
      mMouseOverRect = &mC1;
    else if(mC2.Contains(x, y))
      mMouseOverRect = &mC2;
    else
      mMouseOverRect = nullptr;
    SetDirty(false);
  }

private:
  IRECT mP1, mP2;
  IRECT mC1, mC2;
  IRECT* mMouseOverRect = nullptr;
};
