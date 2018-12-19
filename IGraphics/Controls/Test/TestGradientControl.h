/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"

class TestGradientControl : public IKnobControlBase
{
public:
  TestGradientControl(IGEditorDelegate& dlg, IRECT rect, int paramIdx = kNoParameter)
  : IKnobControlBase(dlg, rect, paramIdx)
  {
    RandomiseGradient();
  }

  void Draw(IGraphics& g) override
  {
    if (g.HasPathSupport())
    {
      double cr = mValue * (mRECT.H() / 2.0);
      g.PathRoundRect(mRECT.GetPadded(-2), cr);
      IFillOptions fillOptions;
      IStrokeOptions strokeOptions;
      fillOptions.mPreserve = true;
      g.PathFill(mPattern, fillOptions);
      g.PathStroke(IColor(255, 0, 0, 0), 3, strokeOptions);
    }
    else
      g.DrawText(mText, "UNSUPPORTED", mRECT);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    RandomiseGradient();
    SetDirty(false);
  }

  void RandomiseGradient()
  {
    //IPattern tmp(kLinearPattern);
    //tmp.SetTransform(1.0/mRECT.W(), 0, 0, 1.0/mRECT.W(), 1.0/mRECT.W()*-mRECT.L, 1.0/mRECT.W()*-mRECT.T);
    IPattern tmp(kSolidPattern);

    if (std::rand() & 0x100)
      tmp = IPattern::CreateRadialGradient(mRECT.MW(), mRECT.MH(), mRECT.MH());
    else
      tmp = IPattern::CreateLinearGradient(mRECT.L, mRECT.MH(), mRECT.L + mRECT.W() * 0.5, mRECT.MH());

    tmp.mExtend = (std::rand() & 0x10) ? ((std::rand() & 0x1000) ? kExtendNone : kExtendPad) : ((std::rand() & 0x1000) ? kExtendRepeat : kExtendReflect);

    tmp.AddStop(IColor::GetRandomColor(), 0.0);
    tmp.AddStop(IColor::GetRandomColor(), 0.1);
    tmp.AddStop(IColor::GetRandomColor(), 0.4);
    tmp.AddStop(IColor::GetRandomColor(), 0.6);
    tmp.AddStop(IColor::GetRandomColor(), 1.0);

    mPattern = tmp;
  }

private:
  IPattern mPattern = IPattern(kLinearPattern);
};
