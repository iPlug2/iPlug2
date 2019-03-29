/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestBlendControl
 */

#include "IControl.h"

/** Control to test blend methods
 *   @ingroup TestControls */
class TestBlendControl : public IKnobControlBase
                       , public IBitmapBase
{
public:
  TestBlendControl(IRECT bounds, const IBitmap& bitmap)
  : IKnobControlBase(bounds)
  , IBitmapBase(bitmap)
  {
    SetTooltip("TestBlendControl");
  }

  void Draw(IGraphics& g) override
  {
    const float alpha = static_cast<float>(mValue);

    int cell = 0;
    auto nextCell = [&]() {
      return mRECT.GetGridCell(cell++, 4, 4);
    };

    auto drawBlendPic = [this](IGraphics& g, IRECT r, EBlendType blend, const char* name, float alpha)
    {
      IBlend blendMode { blend, alpha };
      g.FillCircle(IColor(128, 255, 0, 0), r.MW(), r.MH(), r.W() / 2.0);
      g.DrawFittedBitmap(mBitmap, r, &blendMode);
      g.DrawText(mText, name, r);
    };

    g.StartLayer(mRECT);
    drawBlendPic(g, nextCell(), kBlendDefault, "Default", alpha);
    drawBlendPic(g, nextCell(), kBlendClobber, "Clobber", alpha);
    drawBlendPic(g, nextCell(), kBlendAdd, "Add", alpha);
    drawBlendPic(g, nextCell(), kBlendXOR, "XOR", alpha);
    drawBlendPic(g, nextCell(), kBlendSourceOver, "Src Over", alpha);
    drawBlendPic(g, nextCell(), kBlendSourceIn, "Src In", alpha);
    drawBlendPic(g, nextCell(), kBlendSourceOut, "Src Out", alpha);
    drawBlendPic(g, nextCell(), kBlendSourceAtop, "Src Atop", alpha);
    drawBlendPic(g, nextCell(), kBlendDestOver, "Dst Over", alpha);
    drawBlendPic(g, nextCell(), kBlendDestIn, "Dst In", alpha);
    drawBlendPic(g, nextCell(), kBlendDestOut, "Dst Out", alpha);
    drawBlendPic(g, nextCell(), kBlendDestAtop, "Dst Atop", alpha);
    mLayer = g.EndLayer();
    g.DrawLayer(mLayer);
  }
    
private:
    
  ILayerPtr mLayer;
};
