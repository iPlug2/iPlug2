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
#if defined IGRAPHICS_NANOVG
    NVGcontext* vg = (NVGcontext*) g.GetDrawContext();
    
    IRECT r1 = mRECT.GetCentredInside(100);
    
    nvgSave(vg);
    nvgTranslate(vg, r1.MW(), r1.MH());
    nvgRotate(vg, nvgDegToRad(30));
    nvgTranslate(vg, -r1.MW(), -r1.MH());
    nvgBeginPath(vg);
    nvgRect(vg, r1.L, r1.T, r1.W(), r1.H());
    nvgFillColor(vg, nvgRGBA(255,192,0,255));
    nvgFill(vg);
    nvgRestore(vg);
#elif defined IGRAPHICS_LICE
#elif defined IGRAPHICS_CAIRO
#elif defined IGRAPHICS_AGG

#endif
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
  }
};
