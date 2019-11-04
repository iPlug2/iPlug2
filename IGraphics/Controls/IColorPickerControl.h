/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IColorPickerControl
 */

#include "IControl.h"
#ifdef IGRAPHICS_NANOVG
#include "nanovg.h"
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control for choosing a color
 * @ingroup IControls */
class IColorPickerControl : public IControl
{
public:
  IColorPickerControl(const IRECT& bounds)
  : IControl(bounds)
  {
  }

  void Draw(IGraphics& g) override
  {
#ifdef IGRAPHICS_NANOVG
    NVGcontext* vg = (NVGcontext* ) g.GetDrawContext();
    float x = mRECT.L;
    float y = mRECT.T;
    float w = mRECT.W();
    float h = mRECT.H();

    int i;
    float r0, r1, ax,ay, bx,by, cx,cy, aeps, r;
    float hue = 0.;
    NVGpaint paint;

    nvgSave(vg);

    /*  nvgBeginPath(vg);
     nvgRect(vg, x,y,w,h);
     nvgFillColor(vg, nvgRGBA(255,0,0,128));
     nvgFill(vg);*/

    cx = x + w*0.5f;
    cy = y + h*0.5f;
    r1 = (w < h ? w : h) * 0.5f - 5.0f;
    r0 = r1 - 20.0f;
    aeps = 0.5f / r1;  // half a pixel arc length in radians (2pi cancels out).

    for (i = 0; i < 6; i++) {
      float a0 = (float)i / 6.0f * NVG_PI * 2.0f - aeps;
      float a1 = (float)(i+1.0f) / 6.0f * NVG_PI * 2.0f + aeps;
      nvgBeginPath(vg);
      nvgArc(vg, cx,cy, r0, a0, a1, NVG_CW);
      nvgArc(vg, cx,cy, r1, a1, a0, NVG_CCW);
      nvgClosePath(vg);
      ax = cx + cosf(a0) * (r0+r1)*0.5f;
      ay = cy + sinf(a0) * (r0+r1)*0.5f;
      bx = cx + cosf(a1) * (r0+r1)*0.5f;
      by = cy + sinf(a1) * (r0+r1)*0.5f;
      paint = nvgLinearGradient(vg, ax,ay, bx,by, nvgHSLA(a0/(NVG_PI*2),1.0f,0.55f,255), nvgHSLA(a1/(NVG_PI*2),1.0f,0.55f,255));
      nvgFillPaint(vg, paint);
      nvgFill(vg);
    }

    nvgBeginPath(vg);
    nvgCircle(vg, cx,cy, r0-0.5f);
    nvgCircle(vg, cx,cy, r1+0.5f);
    nvgStrokeColor(vg, nvgRGBA(0,0,0,64));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);

    // Selector
    nvgSave(vg);
    nvgTranslate(vg, cx,cy);
    nvgRotate(vg, hue*NVG_PI*2);

    // Marker on
    nvgStrokeWidth(vg, 2.0f);
    nvgBeginPath(vg);
    nvgRect(vg, r0-1,-3,r1-r0+2,6);
    nvgStrokeColor(vg, nvgRGBA(255,255,255,192));
    nvgStroke(vg);

    paint = nvgBoxGradient(vg, r0-3,-5,r1-r0+6,10, 2,4, nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, r0-2-10,-4-10,r1-r0+4+20,8+20);
    nvgRect(vg, r0-2,-4,r1-r0+4,8);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, paint);
    nvgFill(vg);

    // Center triangle
    r = r0 - 6;
    ax = cosf(120.0f/180.0f*NVG_PI) * r;
    ay = sinf(120.0f/180.0f*NVG_PI) * r;
    bx = cosf(-120.0f/180.0f*NVG_PI) * r;
    by = sinf(-120.0f/180.0f*NVG_PI) * r;
    nvgBeginPath(vg);
    nvgMoveTo(vg, r,0);
    nvgLineTo(vg, ax,ay);
    nvgLineTo(vg, bx,by);
    nvgClosePath(vg);
    paint = nvgLinearGradient(vg, r,0, ax,ay, nvgHSLA(hue,1.0f,0.5f,255), nvgRGBA(255,255,255,255));
    nvgFillPaint(vg, paint);
    nvgFill(vg);
    paint = nvgLinearGradient(vg, (r+ax)*0.5f,(0+ay)*0.5f, bx,by, nvgRGBA(0,0,0,0), nvgRGBA(0,0,0,255));
    nvgFillPaint(vg, paint);
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(0,0,0,64));
    nvgStroke(vg);

    // Select circle on triangle
    ax = cosf(120.0f/180.0f*NVG_PI) * r*0.3f;
    ay = sinf(120.0f/180.0f*NVG_PI) * r*0.4f;
    nvgStrokeWidth(vg, 2.0f);
    nvgBeginPath(vg);
    nvgCircle(vg, ax,ay,5);
    nvgStrokeColor(vg, nvgRGBA(255,255,255,192));
    nvgStroke(vg);

    paint = nvgRadialGradient(vg, ax,ay, 7,9, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, ax-20,ay-20,40,40);
    nvgCircle(vg, ax,ay,7);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, paint);
    nvgFill(vg);

    nvgRestore(vg);

    nvgRestore(vg);
#endif
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
  }
private:
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
