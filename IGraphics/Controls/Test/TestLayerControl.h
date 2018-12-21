/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"

class TestLayerControl : public IKnobControlBase
{
public:
  TestLayerControl(IGEditorDelegate& dlg, IRECT rect)
  : IKnobControlBase(dlg, rect, kNoParameter)
  {
  }

  void Draw(IGraphics& g) override
  {
    if (mDrawBackground)
    {
      if (!g.CheckLayer(mLayer))
      {
        IText text;
        text.mVAlign = IText::kVAlignTop;
        text.mSize = 15;
        g.StartLayer(mRECT);
        g.FillRoundRect(COLOR_LIGHT_GRAY, mRECT.GetPadded(-5.5f), mRECT.W() / 4.0);
        g.DrawText(text, "Cached Layer", mRECT.GetPadded(-10));
        mLayer = g.EndLayer();
      }
     
      g.DrawLayer(mLayer);
    }
        
    g.FillCircle(COLOR_BLUE, mRECT.MW(), mRECT.MH(), mRECT.H() / 4.0);
    g.DrawRadialLine(COLOR_BLACK, mRECT.MW(), mRECT.MH(), -120.0 + mValue * 240.0, 0.0, mRECT.H() / 4.0, nullptr, 3.0);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mLayer->Invalidate();
    mDrawBackground = !mDrawBackground;
    SetDirty(false);
  }

private:
  ILayerPtr mLayer;
  bool mDrawBackground = true;
};
