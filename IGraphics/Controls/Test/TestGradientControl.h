/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestGradientControl
 */

#include "IControl.h"

/** Control to test drawing gradients with path based drawing backends
 *   @ingroup TestControls */
class TestGradientControl : public IKnobControlBase
{
public:
  TestGradientControl(IRECT rect, int paramIdx = kNoParameter)
  : IKnobControlBase(rect, paramIdx)
  {
    SetTooltip("TestGradientControl");
    RandomiseGradient();
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);

    if (g.HasPathSupport())
    {
      float cr = static_cast<float>(GetValue()) * (mRECT.H() / 2.f);
      g.PathRoundRect(mRECT.GetPadded(-2.f), cr);
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
  
  void OnResize() override
  {
    RandomiseGradient();
  }

private:
  
  void RandomiseGradient()
  {
    //IPattern tmp(EPatternType::Linear);
    //tmp.SetTransform(1.0/mRECT.W(), 0, 0, 1.0/mRECT.W(), 1.0/mRECT.W()*-mRECT.L, 1.0/mRECT.W()*-mRECT.T);
    IPattern tmp(EPatternType::Solid);

    if (std::rand() & 0x100)
      tmp = IPattern::CreateRadialGradient(mRECT.MW(), mRECT.MH(), mRECT.MH());
    else
      tmp = IPattern::CreateLinearGradient(mRECT.L, mRECT.MH(), mRECT.L + mRECT.W() * 0.5f, mRECT.MH());

    tmp.mExtend = (std::rand() & 0x10) ? ((std::rand() & 0x1000) ? EPatternExtend::None : EPatternExtend::Pad) : ((std::rand() & 0x1000) ? EPatternExtend::Repeat : EPatternExtend::Reflect);

    tmp.AddStop(IColor::GetRandomColor(), 0.0);
    tmp.AddStop(IColor::GetRandomColor(), 0.1);
    tmp.AddStop(IColor::GetRandomColor(), 0.4);
    tmp.AddStop(IColor::GetRandomColor(), 0.6);
    tmp.AddStop(IColor::GetRandomColor(), 1.0);

    mPattern = tmp;
  }

  IPattern mPattern = IPattern(EPatternType::Linear);
};
