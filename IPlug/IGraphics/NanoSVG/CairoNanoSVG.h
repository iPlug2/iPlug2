#pragma once

#include "nanosvg.h"
#include "cairo/cairo.h"

namespace CairoNanoSVGRender
{
  static void GetNVGColor(const unsigned int color, float& R, float& G, float& B, float& A)
  {
    R = ((color >> 0) & 0xFF) / 255.0;
    G = ((color >> 8) & 0xFF) / 255.0;
    B = ((color >> 16) & 0xFF) / 255.0;
    A = ((color >> 24) & 0xFF) / 255.0;
  }
  
  static void SetSource(cairo_t* pCtx, const NSVGpaint& paint, float opacity)
  {
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
      {
        float A, R, G, B;
        GetNVGColor(paint.color, R, G, B, A);
        cairo_set_source_rgba(pCtx, R, G, B, opacity); // A?
        return;
      }
      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      {
        NSVGgradient* pGrad = paint.gradient;
        
        // float fx, fy; ??
        
        cairo_pattern_t* pPattern;
        
        if (paint.type == NSVG_PAINT_LINEAR_GRADIENT)
          pPattern = cairo_pattern_create_linear(0, 0, 0, 1);
        else
          pPattern = cairo_pattern_create_radial(0, 0, 1, 1, 0, 1);
        
        switch (pGrad->spread)
        {
          case NSVG_SPREAD_PAD:     cairo_pattern_set_extend(pPattern, CAIRO_EXTEND_PAD);    break;
          case NSVG_SPREAD_REFLECT:   cairo_pattern_set_extend(pPattern, CAIRO_EXTEND_REFLECT);  break;
          case NSVG_SPREAD_REPEAT:  cairo_pattern_set_extend(pPattern, CAIRO_EXTEND_REPEAT);   break;
        }
        
        for (int i = 0; i < pGrad->nstops; i++)
        {
          const NSVGgradientStop& stop = pGrad->stops[i];
          float A, R, G, B;
          GetNVGColor(stop.color, R, G, B, A);
          cairo_pattern_add_color_stop_rgb(pPattern, stop.offset, R, G, B);
        }
        
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix, pGrad->xform[0], pGrad->xform[1], pGrad->xform[2], pGrad->xform[3], pGrad->xform[4], pGrad->xform[5]);
        cairo_pattern_set_matrix(pPattern, &matrix);
        cairo_set_source(pCtx, pPattern);
        cairo_pattern_destroy(pPattern);
        return;
      }
      default:
        cairo_set_source_rgba(pCtx, 0, 0, 0, opacity);
        return;
    }
  }
  
  static void RenderNanoSVG(cairo_t* pCtx, NSVGimage *image)
  {
    for (NSVGshape* pShape = image->shapes; pShape; pShape = pShape->next)
    {
      if (!(pShape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
      for (NSVGpath* pPath = pShape->paths; pPath; pPath = pPath->next)
      {
        cairo_move_to(pCtx, pPath->pts[0], pPath->pts[1]);
        
        for (int i = 0; i < pPath->npts - 1; i += 3)
        {
          float *p = pPath->pts + i * 2 + 2;
          cairo_curve_to(pCtx, p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        
        if (pPath->closed)
          cairo_close_path(pCtx);
      }
      
      // Fill
      
      if (pShape->fill.type != NSVG_PAINT_NONE)
      {
        if (pShape->fillRule == NSVG_FILLRULE_EVENODD)
          cairo_set_fill_rule(pCtx, CAIRO_FILL_RULE_EVEN_ODD);
        else
          cairo_set_fill_rule(pCtx, CAIRO_FILL_RULE_WINDING);
        SetSource(pCtx, pShape->fill, pShape->opacity);
        
        if (pShape->stroke.type != NSVG_PAINT_NONE)
          cairo_fill_preserve(pCtx);
        else
          cairo_fill(pCtx);
      }
      
      // Stroke
      
      if (pShape->stroke.type != NSVG_PAINT_NONE)
      {
        cairo_set_line_width(pCtx, pShape->strokeWidth);
        cairo_set_miter_limit(pCtx, pShape->miterLimit);
        
        switch (pShape->strokeLineCap)
        {
          case NSVG_CAP_BUTT:   cairo_set_line_cap(pCtx, CAIRO_LINE_CAP_BUTT);  break;
          case NSVG_CAP_ROUND:  cairo_set_line_cap(pCtx, CAIRO_LINE_CAP_ROUND);   break;
          case NSVG_CAP_SQUARE: cairo_set_line_cap(pCtx, CAIRO_LINE_CAP_SQUARE);  break;
        }
        
        switch (pShape->strokeLineJoin)
        {
          case NSVG_JOIN_MITER:   cairo_set_line_join(pCtx, CAIRO_LINE_JOIN_MITER);  break;
          case NSVG_JOIN_ROUND:   cairo_set_line_join(pCtx, CAIRO_LINE_JOIN_ROUND);   break;
          case NSVG_JOIN_BEVEL:   cairo_set_line_join(pCtx, CAIRO_LINE_JOIN_BEVEL);  break;
        }
        
        double dashArray[8];
        
        for (int i = 0; i < pShape->strokeDashCount; i++)
          dashArray[i] = pShape->strokeDashArray[i];
        
        cairo_set_dash(pCtx, dashArray, pShape->strokeDashCount, pShape->strokeDashOffset);
        
        SetSource(pCtx, pShape->stroke, pShape->opacity);
        cairo_stroke(pCtx);
      }
    }
  }
}

