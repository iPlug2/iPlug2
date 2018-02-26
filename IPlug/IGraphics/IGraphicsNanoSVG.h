#pragma once

#include "nanosvg.h"
#include "IGraphics.h"

namespace NanoSVGRenderer
{
  static IPattern GetPattern(const NSVGpaint& paint, float opacity)
  {
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
        return IColor(opacity, (paint.color >> 0) & 0xFF, (paint.color >> 8) & 0xFF, (paint.color >> 16) & 0xFF);
    
      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      {
        NSVGgradient* pGrad = paint.gradient;
        
        IPattern pattern(paint.type == NSVG_PAINT_LINEAR_GRADIENT ? kLinearPattern : kRadialPattern);
        
        // Set Extend Rule
        
        switch (pGrad->spread)
        {
          case NSVG_SPREAD_PAD:       pattern.mExtend = kExtendPad;       break;
          case NSVG_SPREAD_REFLECT:   pattern.mExtend = kExtendReflect;   break;
          case NSVG_SPREAD_REPEAT:    pattern.mExtend = kExtendRepeat;    break;
        }
        
        // Copy Stops
        
        for (int i = 0; i < pGrad->nstops; i++)
        {
          int color = pGrad->stops[i].color;
          pattern.AddStop(IColor(255, (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF), pGrad->stops[i].offset);
        }
        
        // Copy transform
        
        pattern.SetTransform(pGrad->xform[0], pGrad->xform[1], pGrad->xform[2], pGrad->xform[3], pGrad->xform[4], pGrad->xform[5]);
        
        return pattern;
      }
      default:
        return IColor(opacity, 0, 0, 0);
    }
  }
  
  static void RenderNanoSVG(IGraphics& graphics, NSVGimage *image)
  {
    for (NSVGshape* pShape = image->shapes; pShape; pShape = pShape->next)
    {
      if (!(pShape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
      for (NSVGpath* pPath = pShape->paths; pPath; pPath = pPath->next)
      {
        graphics.PathMoveTo(pPath->pts[0], pPath->pts[1]);
        
        for (int i = 0; i < pPath->npts - 1; i += 3)
        {
          float *p = pPath->pts + i * 2 + 2;
          graphics.PathCurveTo(p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        
        if (pPath->closed)
          graphics.PathClose();
      }
      
      // Fill
      
      if (pShape->fill.type != NSVG_PAINT_NONE)
      {
        IFillOptions options;

        if (pShape->fillRule == NSVG_FILLRULE_EVENODD)
          options.mFillRule = kFillEvenOdd;
        else
          options.mFillRule = kFillWinding;
        
        options.mPreserve = pShape->stroke.type != NSVG_PAINT_NONE;
        graphics.PathFill(GetPattern(pShape->fill, pShape->opacity), options, nullptr);
      }
      
      // Stroke
      
      if (pShape->stroke.type != NSVG_PAINT_NONE)
      {
        IStrokeOptions options;
          
        options.mMiterLimit = pShape->miterLimit;
        
        switch (pShape->strokeLineCap)
        {
          case NSVG_CAP_BUTT:   options.mCapOption = kCapButt;    break;
          case NSVG_CAP_ROUND:  options.mCapOption = kCapRound;   break;
          case NSVG_CAP_SQUARE: options.mCapOption = kCapSquare;  break;
        }
        
        switch (pShape->strokeLineJoin)
        {
          case NSVG_JOIN_MITER:   options.mJoinOption = kJoinMiter;   break;
          case NSVG_JOIN_ROUND:   options.mJoinOption = kJoinRound;   break;
          case NSVG_JOIN_BEVEL:   options.mJoinOption = kJoinBevel;   break;
        }
        
        options.mDash.SetDash(pShape->strokeDashArray, pShape->strokeDashOffset, pShape->strokeDashCount);
         
        graphics.PathStroke(GetPattern(pShape->stroke, pShape->opacity), pShape->strokeWidth, options, nullptr);
      }
    }
  }
}

