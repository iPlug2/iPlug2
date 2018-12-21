/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"
#include "Easing.h"

class TestAnimationControl : public IControl
{
public:
  TestAnimationControl(IGEditorDelegate& dlg, IRECT bounds)
  : IControl(dlg, bounds, kNoParameter)
  {
    SetActionFunction([&](IControl* pCaller) {
      
      SetAnimation([&](IControl* pCaller) {
        auto progress = pCaller->GetAnimationProgress();
        
        if(progress > 1.) {
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
    g.FillRect(mDrawnColor, mDrawnRect);
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
