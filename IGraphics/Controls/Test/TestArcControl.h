/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestArcControl
 */

#include "IControl.h"

/** Control to test drawing arcs
 *   @ingroup TestControls */
class TestArcControl : public IKnobControlBase
{
public:
  TestArcControl(IRECT rect, int paramIdx = kNoParameter, float angle1 = -135.f, float angle2 = 135.f)
  : IKnobControlBase(rect, paramIdx)
  , mAngle1(angle1)
  , mAngle2(angle2)
  {
    SetTooltip("TestArcControl");
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_WHITE, mRECT.GetPadded(-2));
    g.DrawRect(COLOR_BLACK, mRECT.GetPadded(-2));
    float angle = mAngle1 + (float) GetValue() * (mAngle2 - mAngle1);
    g.FillArc(COLOR_BLUE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44f, mAngle1, angle);
    g.DrawArc(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44f, mAngle1, angle);
    g.DrawRadialLine(COLOR_BLACK, mRECT.MW(), mRECT.MH(), angle, 0.f, mRECT.W() * 0.49f);
    g.FillCircle(COLOR_WHITE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1f);
    g.DrawCircle(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1f);

    angle = DegToRad(angle-90.f);

    float x1 = mRECT.MW() + cosf(angle - 0.3f) * mRECT.W() * 0.3f;
    float y1 = mRECT.MH() + sinf(angle - 0.3f) * mRECT.W() * 0.3f;
    float x2 = mRECT.MW() + cosf(angle + 0.3f) * mRECT.W() * 0.3f;
    float y2 = mRECT.MH() + sinf(angle + 0.3f) * mRECT.W() * 0.3f;
    float x3 = mRECT.MW() + cosf(angle) * mRECT.W() * 0.44f;
    float y3 = mRECT.MH() + sinf(angle) * mRECT.W() * 0.44f;

    g.FillTriangle(COLOR_WHITE, x1, y1, x2, y2, x3, y3);
    g.DrawTriangle(COLOR_BLACK, x1, y1, x2, y2, x3, y3);
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

private:
  float mAngle1;
  float mAngle2;
};
