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
  }

  void Draw(IGraphics& g) override
  {
    g.DrawRoundRect(COLOR_BLACK, mRECT, 5.);
    
    const double value = GetValue();

    if (g.HasPathSupport())
    {
      double r0 = value * (mRECT.H() / 2.0);
      float l = mRECT.L + mRECT.W() * 0.1;
      float r = mRECT.L + mRECT.W() * 0.9;
      float t = mRECT.T + mRECT.H() * 0.1;
      float b = mRECT.T + mRECT.H() * 0.9;
      float l0 = mRECT.L + mRECT.W() * 0.2;
      float t0 = mRECT.T + mRECT.H() * 0.3;
      float b0 = mRECT.T + mRECT.H() * 0.7;
      float mx = mRECT.L + mRECT.W() * 0.43;
      float my = mRECT.MH();

      if (mShape == 0)
      {
        g.PathCircle(mRECT.MW(), mRECT.MH(), r0);
        g.PathCircle(mRECT.MW(), mRECT.MH(), r0 * 0.5);
      }
      else if (mShape == 1)
      {
        float pad1 = (mRECT.W() / 2.0) * (1.0 - GetValue());
        float pad2 = (mRECT.H() / 2.0) * (1.0 - GetValue());
        IRECT size1 = mRECT.GetPadded(-pad1, -pad2, -pad1, -pad2);
        pad1 = (size1.W() / 2.0) * (1.0 - GetValue());
        pad2 = (size1.H() / 2.0) * (1.0 - GetValue());
        IRECT size2 = size1.GetPadded(-pad1, -pad2, -pad1, -pad2);
        g.PathRect(size1);
        g.PathRect(size2);
      }
      else if (mShape == 2)
      {
        float pad1 = (mRECT.W() / 2.0) * (1.0 - GetValue());
        float pad2 = (mRECT.H() / 2.0) * (1.0 - GetValue());
        IRECT size1 = mRECT.GetPadded(-pad1, -pad2, -pad1, -pad2);
        pad1 = (size1.W() / 2.0) * (1.0 - GetValue());
        pad2 = (size1.H() / 2.0) * (1.0 - GetValue());
        IRECT size2 = size1.GetPadded(-pad1, -pad2, -pad1, -pad2);
        g.PathRoundRect(size1, size1.H() * 0.125);
        g.PathRoundRect(size2, size2.H() * 0.125);
      }
      else if (mShape == 3)
      {
        g.PathMoveTo(mRECT.L, mRECT.B);
        g.PathCubicBezierTo(mRECT.L + mRECT.W() * 0.125, mRECT.T + mRECT.H() * 0.725, mRECT.L + mRECT.W() * 0.25, mRECT.T + mRECT.H() * 0.35, mRECT.MW(), mRECT.MH());
        g.PathLineTo(mRECT.MW(), mRECT.B);
        g.PathClose();
      }
      else if (mShape == 4)
      {
        g.PathMoveTo(l, b);
        g.PathLineTo(l, t);
        g.PathLineTo(r, t);
        g.PathLineTo(l0, b0);
        g.PathLineTo(l0, t0);
        g.PathLineTo(r, b);
        g.PathClose();
      }
      else if (mShape == 5)
      {
        g.PathMoveTo(l, b);
        g.PathLineTo(l, t);
        g.PathLineTo(r, t);
        g.PathLineTo(mx, my);
        g.PathLineTo(l0, t0);
        g.PathLineTo(l0, b0);
        g.PathLineTo(mx, my);
        g.PathLineTo(r, b);
        g.PathClose();
      }
      else if (mShape == 6)
      {
        g.PathMoveTo(l, b);
        g.PathLineTo(l, t);
        g.PathLineTo(r, t);
        g.PathLineTo(mx, my);
        g.PathLineTo(r, b);
        g.PathClose();
        g.PathMoveTo(l0, b0);
        g.PathLineTo(l0, t0);
        g.PathLineTo(mx, my);
        g.PathClose();
      }
      else if (mShape == 7)
      {
        g.PathMoveTo(l, b);
        g.PathLineTo(l, t);
        g.PathLineTo(r, t);
        g.PathLineTo(mx, my);
        g.PathLineTo(r, b);
        g.PathClose();
        g.PathMoveTo(l0, t0);
        g.PathLineTo(l0, b0);
        g.PathLineTo(mx, my);
        g.PathClose();
      }
      else if (mShape == 8)
      {
        float centerX = mRECT.MW();
        float centerY = mRECT.MH();
        float radius = mRECT.W() * 0.25f;
        float width = radius * 0.75f;
        float startAngle = -90.0f;
        float endAngle = +90.0f;
          
        g.PathArc(centerX, centerY, radius - width * 0.5f, startAngle, endAngle);
        g.PathArc(centerX, centerY, radius + width * 0.5f, endAngle, startAngle, EWinding::CCW);
        g.PathClose();
      }
      else
      {
        float centerX = mRECT.MW();
        float centerY = mRECT.MH();
        float radius = mRECT.W() * 0.25f;
        float width = radius * 0.75f;
        float startAngle = -90.0f;
        float endAngle = +90.0f;
          
        g.PathArc(centerX, centerY, radius - width * 0.5f, startAngle, endAngle);
        g.PathArc(centerX, centerY, radius + width * 0.5f, endAngle, startAngle, EWinding::CW);
        g.PathClose();
      }
            
        
      IFillOptions fillOptions;
      fillOptions.mFillRule = value > 0.5 ? EFillRule::EvenOdd : EFillRule::Winding;
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
    if (++mShape > 9)
      mShape = 0;

    SetDirty(false);
  }

private:
  int mShape;
};
