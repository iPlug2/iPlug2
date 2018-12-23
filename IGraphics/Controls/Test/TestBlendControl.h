/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"

class TestBlendControl : public IKnobControlBase
                       , public IBitmapBase
{
public:
  TestBlendControl(IGEditorDelegate& dlg, IRECT bounds, const IBitmap& bitmap)
  : IKnobControlBase(dlg, bounds)
  , IBitmapBase(bitmap)
  {
    SetTooltip("TestBlendControl");
  }
  
  void Draw(IGraphics& g) override
  {
    const float alpha = static_cast<float>(mValue);
    
    int cell = 0;
    IRECT r;
    auto nextCell = [&]() {
      r = mRECT.GetGridCell(cell++, 2, 2);
      return r;
    };
    
    IBlend bNone {kBlendNone, alpha};
    g.FillEllipse(COLOR_RED, nextCell());
    g.DrawFittedBitmap(mBitmap, r, &bNone);

    IBlend bClobber {kBlendClobber, alpha};
    g.FillEllipse(COLOR_RED, nextCell());
    g.DrawFittedBitmap(mBitmap, r, &bClobber);
    
    IBlend bColorDodge {kBlendColorDodge, alpha};
    g.FillEllipse(COLOR_RED, nextCell());
    g.DrawFittedBitmap(mBitmap, r, &bColorDodge);
    
    IBlend bAdd {kBlendAdd, alpha};
    g.FillEllipse(COLOR_RED, nextCell());
    g.DrawFittedBitmap(mBitmap, r, &bAdd);
  }
};
