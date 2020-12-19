/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestDrawContextControl
 */

#include "IControl.h"

/** Control to test obtaining a drawing API (NanoVG, Skia, Canvas) context and using that API within an IControl
 *   @ingroup TestControls */
class TestDrawContextControl : public IControl
{
public:
  TestDrawContextControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetTooltip("TestDrawContextControl");
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);

    IRECT r1 = mRECT.GetCentredInside(100);

#if defined IGRAPHICS_NANOVG
    NVGcontext* vg = (NVGcontext*) g.GetDrawContext();
    nvgSave(vg);
    nvgTranslate(vg, r1.MW(), r1.MH());
    nvgRotate(vg, nvgDegToRad(30));
    nvgTranslate(vg, -r1.MW(), -r1.MH());
    nvgBeginPath(vg);
    nvgRect(vg, r1.L, r1.T, r1.W(), r1.H());
    nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
    nvgFill(vg);
    nvgRestore(vg);
#elif defined IGRAPHICS_SKIA
    SkCanvas* canvas = (SkCanvas*) g.GetDrawContext();
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorRED);
    
    SkRect rect = SkiaRect(r1);
    canvas->translate(r1.MW(), r1.MH());
    canvas->rotate(30);
    canvas->translate(-r1.MW(), -r1.MH());
    canvas->drawRect(rect, paint);
#elif defined IGRAPHICS_CANVAS
    
#else
    g.DrawText(mText, "UNSUPPORTED", mRECT);
#endif
  }
};
