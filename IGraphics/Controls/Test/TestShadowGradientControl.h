/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestShadowGradientControl
 */

#include "IControl.h"

/** Control to test drawing shadows with gradients
 *   @ingroup TestControls */
class TestShadowGradientControl : public IControl
{
public:
    
  TestShadowGradientControl(const IRECT& rect)
  : IControl(rect)
  {
  }
    
  void Draw(IGraphics& g) override
  {
    IPattern pattern = IPattern::CreateLinearGradient(mRECT.L + mRECT.W() / 2.f, mRECT.T + mRECT.H() / 2.f, mRECT.R, mRECT.B);
    pattern.AddStop(COLOR_BLACK, 0.f);
    pattern.AddStop(COLOR_WHITE, 1.f);
        
    IShadow shadow(pattern, 0.f, mRECT.W() / 2.f, mRECT.H() / 2.f, 1.f, false);
    IRECT bounds(mRECT.L, mRECT.T, mRECT.L + mRECT.W() / 2.f, mRECT.T + mRECT.H() / 2.f);
        
    g.StartLayer(this, mRECT);
    g.FillRect(COLOR_WHITE, bounds);
    mLayer = g.EndLayer();
    g.ApplyLayerDropShadow(mLayer, shadow);
    g.DrawLayer(mLayer);
  }
    
private:
  ILayerPtr mLayer;
};
