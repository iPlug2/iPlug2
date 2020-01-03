/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc GFXLabelControl
 */

#include "IControl.h"

/** Control to display the graphics backend
 *   @ingroup TestControls */
class GFXLabelControl : public IControl
{
public:
  GFXLabelControl(const IRECT& rect)
  : IControl(rect)
  {
    mIgnoreMouse = true;
  }
  
  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
//#ifdef IGRAPHICS_NANOVG
//      static IText text {14, COLOR_WHITE, "Roboto-Regular", EAlign::Center, EVAlign::Middle, 0};
//#else
      static IText text {34, COLOR_WHITE, "Roboto-Regular", EAlign::Center, EVAlign::Top, 0};
//#endif
      IRECT r = mRECT;
      g.MeasureText(text, g.GetDrawingAPIStr(), r);
      r.Pad(20, 0, 20, 0);
      
      g.FillRect(COLOR_BLACK, r);
      
      g.DrawText(text, g.GetDrawingAPIStr(), mRECT);
      mLayer = g.EndLayer();
      IShadow shadow(COLOR_BLACK_DROP_SHADOW, 10.0, 3.0, 6.0, 0.7f, true);
      g.ApplyLayerDropShadow(mLayer, shadow);
    }
    
    g.DrawLayer(mLayer);
//    g.DrawRotatedLayer(mLayer, 45);
  }
  
private:
  ILayerPtr mLayer;
};
