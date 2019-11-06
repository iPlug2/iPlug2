/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestAnimationControl
 */

#include "IControl.h"
#include "Easing.h"

/** Control to test animation
 *   @ingroup TestControls */
class TestAnimationControl : public IControl
{
public:
  TestAnimationControl(const IRECT& bounds)
  : IControl(bounds, kNoParameter)
  {
    SetTooltip("TestAnimationControl");

    SetActionFunction([&](IControl* pCaller) {

      SetAnimation([&](IControl* pCaller) {
        auto progress = static_cast<float>(pCaller->GetAnimationProgress());

        if(progress > 1.f) {
          pCaller->OnEndAnimation();
          return;
        }

        IRECT::LinearInterpolateBetween(mStartRect, mEndRect, mDrawnRect, EaseQuadraticIn(progress));
        IColor::LinearInterpolateBetween(mStartColor, mEndColor, mDrawnColor, progress);
      },
      1000);
    });

    mStartRect = mDrawnRect = mRECT.GetRandomSubRect();
    mEndRect = mRECT.GetRandomSubRect();
    mStartColor = mDrawnColor = IColor::GetRandomColor();
    mEndColor = IColor::GetRandomColor();
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);
    g.FillRect(mDrawnColor, mDrawnRect);
    g.DrawText(mText, "Click to animate", mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mEndRect = mRECT.GetRandomSubRect();
    mEndColor = IColor::GetRandomColor();

    SetDirty(true);
  }

  void OnEndAnimation() override
  {
    mStartRect = mEndRect;
    IControl::OnEndAnimation();
  }

private:
  IRECT mStartRect, mEndRect, mDrawnRect;
  IColor mStartColor, mEndColor, mDrawnColor;
};
