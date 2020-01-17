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
  TestBlendControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx)
  : IKnobControlBase(bounds, paramIdx)
  , IBitmapBase(bitmap)
  {
    SetTooltip("TestBlendControl");
    mText.mSize = 12;
  }

  void Draw(IGraphics& g) override
  {
    const float alpha = (float) GetValue();

    int cell = 0;
    auto nextCell = [&]() {
      return mRECT.GetGridCell(cell++, 4, 4);
    };

    auto drawBlendPic = [this](IGraphics& g, IRECT r, EBlend blend, const char* name, float alpha)
    {
      IBlend blendMode { blend, alpha };
      g.FillCircle(IColor(128, 255, 0, 0), r.MW(), r.MH(), r.W() / 2.f);
      g.DrawFittedBitmap(mBitmap, r, &blendMode);
      g.DrawText(mText, name, r);
    };

    g.StartLayer(this, mRECT);
    drawBlendPic(g, nextCell(), EBlend::Default, "Default", alpha);
    drawBlendPic(g, nextCell(), EBlend::Clobber, "Clobber", alpha);
    drawBlendPic(g, nextCell(), EBlend::Add, "Add", alpha);
    drawBlendPic(g, nextCell(), EBlend::XOR, "XOR", alpha);
    drawBlendPic(g, nextCell(), EBlend::SourceOver, "Src Over", alpha);
    drawBlendPic(g, nextCell(), EBlend::SourceIn, "Src In", alpha);
    drawBlendPic(g, nextCell(), EBlend::SourceOut, "Src Out", alpha);
    drawBlendPic(g, nextCell(), EBlend::SourceAtop, "Src Atop", alpha);
    drawBlendPic(g, nextCell(), EBlend::DestOver, "Dst Over", alpha);
    drawBlendPic(g, nextCell(), EBlend::DestIn, "Dst In", alpha);
    drawBlendPic(g, nextCell(), EBlend::DestOut, "Dst Out", alpha);
    drawBlendPic(g, nextCell(), EBlend::DestAtop, "Dst Atop", alpha);
    mLayer = g.EndLayer();
    g.DrawLayer(mLayer);
  }
    
private:
    
  ILayerPtr mLayer;
};
