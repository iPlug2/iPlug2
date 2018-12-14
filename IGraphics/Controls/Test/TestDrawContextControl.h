/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"

class TestDrawContextControl : public IControl
{
public:
  TestDrawContextControl(IGEditorDelegate& dlg, IRECT bounds)
  : IControl(dlg, bounds)
  {
  }

  void Draw(IGraphics& g) override
  {
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
#elif defined IGRAPHICS_LICE
#elif defined IGRAPHICS_CAIRO
    cairo_t* cr = (cairo_t*) g.GetDrawContext();
    
    cairo_save(cr);
    cairo_translate(cr, r1.MW(), r1.MH());
    cairo_rotate(cr, DegToRad(30.f));
    cairo_translate(cr, -r1.MW(), -r1.MH());
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, 1.f, 0.f, 0.f, 1.f);
    cairo_rectangle(cr, r1.L, r1.T, r1.W(), r1.H());
    cairo_fill(cr);
    cairo_close_path(cr);
#elif defined IGRAPHICS_AGG

#endif
  }
};
