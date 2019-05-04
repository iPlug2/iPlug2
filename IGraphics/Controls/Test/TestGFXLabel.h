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
  GFXLabelControl(IRECT rect)
  : IControl(rect)
  {
  }
  
  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(mRECT);
      static IText text {14, COLOR_WHITE, "Roboto-Regular", IText::kAlignCenter, IText::kVAlignMiddle, 0};
      g.FillRect(COLOR_BLACK, mRECT.GetMidVPadded(text.mSize/2.));
      g.DrawText(text, g.GetDrawingAPIStr(), mRECT);
      mLayer = g.EndLayer();
    }
    
    g.DrawRotatedLayer(mLayer, 45);
  }
  
private:
  ILayerPtr mLayer;
};
