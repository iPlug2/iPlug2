#pragma once

#include "nanovg.h"
#include "nanosvg.h"

/* portions of this code thanks to VCVRack */

namespace NanoVGNanoSVGRender
{
  static NVGcolor GetNVGColor(uint32_t color)
  {
    return nvgRGBA((color >> 0) & 0xff,
                   (color >> 8) & 0xff,
                   (color >> 16) & 0xff,
                   (color >> 24) & 0xff);
  }
  
  static void SetSource(NVGcontext* pCtx, const NSVGpaint& paint, float opacity)
  {
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
      {
        NVGcolor color = GetNVGColor(paint.color);
        nvgStrokeColor(pCtx, color);
        nvgFillColor(pCtx, color);
        return;
      }
      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      {
        NSVGgradient* pGradient = paint.gradient;
        NVGcolor icol = GetNVGColor(pGradient->stops[0].color);
        NVGcolor ocol = GetNVGColor(pGradient->stops[pGradient->nstops - 1].color);
        NVGpaint vgpaint;
        
        float inverse[6];
        float s[2];
        float e[2];
        
        nvgTransformInverse(inverse, pGradient->xform);
        nvgTransformPoint(&s[0], &s[1], inverse, 0, 0);
        nvgTransformPoint(&e[0], &e[1], inverse, 0, 1);
        
        if(paint.type == NSVG_PAINT_LINEAR_GRADIENT)
          vgpaint = nvgLinearGradient(pCtx, s[0], s[1], e[0], e[1], icol, ocol);
        else
          vgpaint = nvgRadialGradient(pCtx, s[0], s[1], 0.0, 160, icol, ocol);
        
        nvgFillPaint(pCtx, vgpaint);
        return;
      }
      default:
        nvgStrokeColor(pCtx, nvgRGBf(0, 0, 0));
        nvgFillColor(pCtx, nvgRGBf(0, 0, 0));
        
    }
  }
  
  static void RenderNanoSVG(NVGcontext* pCtx, NSVGimage* pImage)
  {
    for (NSVGshape* pShape = pImage->shapes; pShape; pShape = pShape->next)
    {
      if (!(pShape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
      for (NSVGpath* pPath = pShape->paths; pPath; pPath = pPath->next)
      {
        nvgBeginPath(pCtx);
        nvgMoveTo(pCtx, pPath->pts[0], pPath->pts[1]);
        
        for (int i = 0; i < pPath->npts - 1; i += 3)
        {
          float *p = pPath->pts + i * 2 + 2;
          nvgBezierTo(pCtx, p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        
        if (pPath->closed)
          nvgClosePath(pCtx);
      }
      
      // Fill
      
      if (pShape->fill.type != NSVG_PAINT_NONE)
      {
        SetSource(pCtx, pShape->fill, pShape->opacity);
        
        if (pShape->stroke.type != NSVG_PAINT_NONE)
        {
          // FIX - check
          nvgSave(pCtx);
          nvgFill(pCtx);
          nvgRestore(pCtx);
        }
        else
          nvgFill(pCtx);
      }
      
      // Stroke
      
      if (pShape->stroke.type != NSVG_PAINT_NONE)
      {
        nvgStrokeWidth(pCtx, pShape->strokeWidth);
        nvgMiterLimit(pCtx, pShape->miterLimit);
        
        switch (pShape->strokeLineCap)
        {
          case NSVG_CAP_BUTT:   nvgLineCap(pCtx, NSVG_CAP_BUTT);  break;
          case NSVG_CAP_ROUND:  nvgLineCap(pCtx, NSVG_CAP_ROUND);   break;
          case NSVG_CAP_SQUARE: nvgLineCap(pCtx, NSVG_CAP_SQUARE);  break;
        }
        
        switch (pShape->strokeLineJoin)
        {
          case NSVG_JOIN_MITER:   nvgLineJoin(pCtx, NVG_MITER);  break;
          case NSVG_JOIN_ROUND:   nvgLineJoin(pCtx, NVG_ROUND);   break;
          case NSVG_JOIN_BEVEL:   nvgLineJoin(pCtx, NVG_BEVEL);  break;
        }
        
        double dashArray[8];
        
        for (int i = 0; i < pShape->strokeDashCount; i++)
          dashArray[i] = pShape->strokeDashArray[i];
        
        //cairo_set_dash(cr, dashArray, pShape->strokeDashCount, pShape->strokeDashOffset);
        
        SetSource(pCtx, pShape->stroke, pShape->opacity);
        nvgStroke(pCtx);
      }
    }
  }
}

