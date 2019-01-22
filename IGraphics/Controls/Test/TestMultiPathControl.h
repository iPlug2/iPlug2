/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestMultiPathControl
 */

#include "IControl.h"

/** Control to test drawing paths in path-based drawing backends
 *   @ingroup TestControls */
class TestMultiPathControl : public IKnobControlBase
{
public:
  TestMultiPathControl(IRECT rect, int paramIdx = kNoParameter)
  : IKnobControlBase(rect, paramIdx)
  , mShape(0)
  {
    SetTooltip("TestMultiPathControl");
    Clamp(0.5, 1.);
  }

  void Draw(IGraphics& g) override
  {
    g.DrawRoundRect(COLOR_BLACK, mRECT, 5.);

    if (g.HasPathSupport())
    {
      double r = mValue * (mRECT.H() / 2.0);
      if (mShape == 0)
      {
        g.PathCircle(mRECT.MW(), mRECT.MH(), r);
        g.PathCircle(mRECT.MW(), mRECT.MH(), r * 0.5);
      }
      else if (mShape == 1)
      {
        float pad1 = (mRECT.W() / 2.0) * (1.0 - mValue);
        float pad2 = (mRECT.H() / 2.0) * (1.0 - mValue);
        IRECT size1 = mRECT.GetPadded(pad1, pad2, -pad1, -pad2);
        pad1 = (size1.W() / 2.0) * (1.0 - mValue);
        pad2 = (size1.H() / 2.0) * (1.0 - mValue);
        IRECT size2 = size1.GetPadded(pad1, pad2, -pad1, -pad2);
        g.PathRect(size1);
        g.PathRect(size2);
      }
      else if (mShape == 2)
      {
        float pad1 = (mRECT.W() / 2.0) * (1.0 - mValue);
        float pad2 = (mRECT.H() / 2.0) * (1.0 - mValue);
        IRECT size1 = mRECT.GetPadded(pad1, pad2, -pad1, -pad2);
        pad1 = (size1.W() / 2.0) * (1.0 - mValue);
        pad2 = (size1.H() / 2.0) * (1.0 - mValue);
        IRECT size2 = size1.GetPadded(pad1, pad2, -pad1, -pad2);
        g.PathRoundRect(size1, size1.H() * 0.125);
        g.PathRoundRect(size2, size2.H() * 0.125);
      }
      else if (mShape == 3)
      {
        g.PathMoveTo(mRECT.L, mRECT.B);
        g.PathCurveTo(mRECT.L + mRECT.W() * 0.125, mRECT.T + mRECT.H() * 0.725, mRECT.L + mRECT.W() * 0.25, mRECT.T + mRECT.H() * 0.35, mRECT.MW(), mRECT.MH());
        g.PathLineTo(mRECT.MW(), mRECT.B);
        g.PathClose();
      }

      IFillOptions fillOptions;
      fillOptions.mFillRule = mValue > 0.5 ? kFillEvenOdd : kFillWinding;
      fillOptions.mPreserve = true;
      IStrokeOptions strokeOptions;
      float dashes[] = { 11, 4, 7 };
      strokeOptions.mDash.SetDash(dashes, 0.0, 2);
      g.PathFill(COLOR_BLACK, fillOptions);
      g.PathStroke(COLOR_WHITE, 1, strokeOptions);
    }
    else
      g.DrawText(mText, "UNSUPPORTED", mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (++mShape > 3)
      mShape = 0;

    SetDirty(false);
  }

private:
  int mShape;
};
