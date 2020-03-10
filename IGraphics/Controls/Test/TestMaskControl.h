/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestMaskControl
 */

#include "IControl.h"

/** Control to display the size of a region
 *   @ingroup TestControls */
class TestMaskControl : public IBitmapControl
{
public:
  TestMaskControl(const IRECT& bounds, const IBitmap& bitmap)
  : IBitmapControl(bounds, bitmap)
  {
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_INDIGO, mRECT);
    IRECT r = mRECT.GetCentredInside(100);
    
    g.StartLayer(this, r);
    g.DrawFittedBitmap(mBitmap, r);
    const IPattern pattern = IPattern::CreateRadialGradient(r.MW(), r.MH(), r.W()/2.f, { {COLOR_BLACK, 0.f}, {COLOR_BLACK, 0.95f}, {COLOR_TRANSPARENT, 1.f} });
    g.PathRect(r);
    g.PathFill(pattern, IFillOptions(), &BLEND_DST_IN);
//    g.FillRect(COLOR_ORANGE, r, &BLEND_DST_OVER); // fill the outside, comment for transparency
    mLayer = g.EndLayer();
    g.DrawLayer(mLayer);
  }

private:
  ILayerPtr mLayer;
};
