/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestPolyControl
 */

#include "IControl.h"

/** Control to test drawing polygons
 *   @ingroup TestControls */
class TestPolyControl : public IKnobControlBase
{
public:
  TestPolyControl(IRECT rect, int paramIdx = kNoParameter)
  : IKnobControlBase(rect, paramIdx)
  {
    SetTooltip("TestPolyControl");
  }

  void Draw(IGraphics& g) override
  {
    float xarray[32];
    float yarray[32];
    int npoints = 3 + (int) roundf((float) mValue * 29.f);
    float angle = (-0.75f * (float) PI) + (float) mValue * (1.5f * (float) PI);
    float incr = (2.f * (float) PI) / npoints;
    float cr = (float) mValue * (mRECT.W() / 2.f);

    g.FillRoundRect(COLOR_WHITE, mRECT.GetPadded(-2.f), cr);
    g.DrawRoundRect(COLOR_BLACK, mRECT.GetPadded(-2.f), cr);

    for (int i = 0; i < npoints; i++)
    {
      xarray[i] = mRECT.MW() + sinf(angle + (float) i * incr) * mRECT.W() * 0.45f;
      yarray[i] = mRECT.MH() + cosf(angle + (float) i * incr) * mRECT.W() * 0.45f;
    }

    g.FillConvexPolygon(COLOR_ORANGE, xarray, yarray, npoints);
    g.DrawConvexPolygon(COLOR_BLACK, xarray, yarray, npoints);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->HideMouseCursor();
    IKnobControlBase::OnMouseDown(x, y, mod);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->HideMouseCursor(false);
    IKnobControlBase::OnMouseUp(x, y, mod);
  }
};
