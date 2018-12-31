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
    nextCell();
    g.FillCircle(COLOR_RED, r.MW(), r.MH(), r.W() / 2.0);
    g.DrawFittedBitmap(mBitmap, r, &bNone);
    g.DrawText(mText, "None", r);

    IBlend bClobber {kBlendClobber, alpha};
    nextCell();
    g.FillCircle(COLOR_RED, r.MW(), r.MH(), r.W() / 2.0);
    g.DrawFittedBitmap(mBitmap, r, &bClobber);
    g.DrawText(mText, "Clobber", r);

#ifndef IGRAPHICS_CAIRO
    IBlend bColorDodge {kBlendColorDodge, alpha};
    nextCell();
    g.FillCircle(COLOR_RED, r.MW(), r.MH(), r.W() / 2.0);
    g.DrawFittedBitmap(mBitmap, r, &bColorDodge);
    g.DrawText(mText, "Color Dodge", r);
#endif

    IBlend bAdd {kBlendAdd, alpha};
    nextCell();
    g.FillCircle(COLOR_RED, r.MW(), r.MH(), r.W() / 2.0);
    g.DrawFittedBitmap(mBitmap, r, &bAdd);
    g.DrawText(mText, "Add", r);
  }
};
