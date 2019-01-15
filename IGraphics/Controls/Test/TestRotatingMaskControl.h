/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"


class TestRotatingMaskControl : public IKnobControlBase
{
public:
  TestRotatingMaskControl(float x, float y, IBitmap& base, IBitmap& mask, IBitmap& top, int paramIdx = kNoParameter)
  : IKnobControlBase(IRECT(x, y, base), paramIdx), mBase(base), mMask(mask), mTop(top)
  {
    SetTooltip("TestRotatingMaskControl");
  }

  void Draw(IGraphics& g) override
  {
    double angle = -120 + mValue * (240);
      g.DrawRotatedMask(mBase, mMask, mTop, mRECT.L, mRECT.T, angle);
  }

private:

  IBitmap mBase, mMask, mTop;
};
