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
{
public:
  TestBlendControl(const IRECT& bounds, const IBitmap& srcBitmap, const IBitmap& dstBitmap, int paramIdx)
  : IKnobControlBase(bounds, paramIdx)
  , mSrc(srcBitmap)
  , mDst(dstBitmap)
  {
    SetTooltip("TestBlendControl");
    mText.mSize = 12;
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_WHITE, mRECT);
    const float alpha = (float) GetValue();

    int cell = 0;
    auto nextCell = [&]() {
      return mRECT.GetGridCell(cell++, 4, 4).GetPadded(-5.f);
    };

    auto drawBlendPic = [this](IGraphics& g, IRECT r, EBlend blend, const char* name, float alpha)
    {
      IBlend blendMode { blend, alpha };
      g.DrawFittedBitmap(mDst, r.GetPadded(-2.f));
      g.DrawFittedBitmap(mSrc, r.GetPadded(-2.f), &blendMode);
      g.DrawRect(COLOR_BLACK, r, nullptr, 1.f);
      g.DrawText(mText, name, r.GetFromTop(10.f));
    };

    g.StartLayer(this, mRECT);
    nextCell();
    nextCell();
    drawBlendPic(g, nextCell(), EBlend::SrcOver, "Src Over", alpha);
    drawBlendPic(g, nextCell(), EBlend::DstOver, "Dst Over", alpha);
    drawBlendPic(g, nextCell(), EBlend::SrcIn, "Src In", alpha);
    drawBlendPic(g, nextCell(), EBlend::DstIn, "Dst In", alpha);
    drawBlendPic(g, nextCell(), EBlend::SrcOut, "Src Out", alpha);
    drawBlendPic(g, nextCell(), EBlend::DstOut, "Dst Out", alpha);
    drawBlendPic(g, nextCell(), EBlend::SrcAtop, "Src Atop", alpha);
    drawBlendPic(g, nextCell(), EBlend::DstAtop, "Dst Atop", alpha);
    drawBlendPic(g, nextCell(), EBlend::XOR, "XOR", alpha);
    drawBlendPic(g, nextCell(), EBlend::Add, "Add", alpha);
    mLayer = g.EndLayer();
    g.DrawLayer(mLayer);
  }
    
private:
  IBitmap mSrc;
  IBitmap mDst;
  ILayerPtr mLayer;
};
