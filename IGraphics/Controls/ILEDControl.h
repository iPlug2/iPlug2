/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc ILEDControl
 */

#include "IControl.h"
#include "Easing.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Glowing LED control
 * @ingroup IControls */
class ILEDControl : public IControl
{
public:
  ILEDControl(const IRECT& bounds, float hue = 0.f)
  : IControl(bounds)
  , mHue(hue)
  {
  }
  
  ILEDControl(const IRECT& bounds, const IColor& color)
  : IControl(bounds)
  {
    float s,l,a;
    color.GetHSLA(mHue, s, l, a);
  }

  void Draw(IGraphics& g) override
  {
    const float v = static_cast<float>(GetValue() * 0.65f);
    const IColor c = IColor::FromHSLA(mHue, 1.f, v);
    IRECT innerPart = mRECT.GetCentredInside(mRECT.W()/2.f);
    IRECT flare = innerPart.GetScaledAboutCentre(1.f + v);
    g.FillEllipse(c, innerPart, nullptr);
    g.DrawEllipse(COLOR_BLACK, innerPart, nullptr, 1.f);
    g.PathEllipse(flare);
    IBlend b = {EBlend::Default, v};
    g.PathFill(IPattern::CreateRadialGradient(mRECT.MW(), mRECT.MH(), mRECT.W()/2.f, {{c, 0.f}, {COLOR_TRANSPARENT, 1.f}}), {}, &b);
  }
  
  void TriggerWithDecay(int decayTimeMs)
  {
    SetAnimation([](IControl* pControl){
      auto progress = pControl->GetAnimationProgress();
      
      if(progress > 1.f) {
        pControl->OnEndAnimation();
        return;
      }
      
      pControl->SetValue(EaseCubicIn(1. -progress));
      
    }, decayTimeMs);
  }

private:
  float mHue = 0.f;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
