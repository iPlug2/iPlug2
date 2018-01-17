
#include "CairoNanoSVG.h"

namespace CairoNanoSVGRender
{
  double colorR(unsigned int color)
  {
    return ((color >> 0) & 0xFF) / 255.0;
  }

  double colorG(unsigned int color)
  {
    return ((color >> 8) & 0xFF) / 255.0;
  }

  double colorB(unsigned int color)
  {
    return ((color >> 16) & 0xFF) / 255.0;
  }
  
  void setSource(cairo_t* cr, const NSVGpaint& paint, float opacity)
  {
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
      {
        float r = colorR(paint.color);
        float g = colorG(paint.color);
        float b = colorB(paint.color);
        cairo_set_source_rgba(cr, r, g, b, opacity);
        break;
      }
      
      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      {
        NSVGgradient *grad = paint.gradient;
        
        // float fx, fy; ??
        
        cairo_pattern_t* pattern;
        
        if (paint.type == NSVG_PAINT_LINEAR_GRADIENT)
          pattern = cairo_pattern_create_linear(0, 0, 0, 1);
        else
          pattern = cairo_pattern_create_radial(0, 0, 1, 1, 0, 1);
        
        switch (grad->spread)
        {
          case NSVG_SPREAD_PAD:     cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);    break;
          case NSVG_SPREAD_REFLECT:   cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT);  break;
          case NSVG_SPREAD_REPEAT:  cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);   break;
        }
        
        for (int i = 0; i < grad->nstops; i++)
        {
          const NSVGgradientStop& stop = grad->stops[i];
          cairo_pattern_add_color_stop_rgb(pattern, stop.offset, colorR(stop.color), colorG(stop.color), colorB(stop.color));
        }
        
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix, grad->xform[0], grad->xform[1], grad->xform[2], grad->xform[3], grad->xform[4], grad->xform[5]);
        cairo_pattern_set_matrix(pattern, &matrix);
        cairo_set_source(cr, pattern);
        cairo_pattern_destroy(pattern);
      }
      break;
        
      default:
        cairo_set_source_rgba(cr, 0, 0, 0, opacity);
    }
  }
  
  void RenderNanoSVG(cairo_t* cr, NSVGimage *image)
  {
    for (NSVGshape *shape = image->shapes; shape; shape = shape->next)
    {
      if (!(shape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
      for (NSVGpath *path = shape->paths; path; path = path->next)
      {
        cairo_move_to(cr, path->pts[0], path->pts[1]);
        
        for (int i = 0; i < path->npts - 1; i += 3)
        {
          float *p = path->pts + i * 2 + 2;
          cairo_curve_to(cr, p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        
        if (path->closed)
          cairo_close_path(cr);
      }
      
      // Fill
      
      if (shape->fill.type != NSVG_PAINT_NONE)
      {
        if (shape->fillRule == NSVG_FILLRULE_EVENODD)
          cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
        else
          cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
        setSource(cr, shape->fill, shape->opacity);
        
        if (shape->stroke.type != NSVG_PAINT_NONE)
          cairo_fill_preserve(cr);
        else
          cairo_fill(cr);
      }
      
      // Stroke
      
      if (shape->stroke.type != NSVG_PAINT_NONE)
      {
        cairo_set_line_width(cr, shape->strokeWidth);
        cairo_set_miter_limit(cr, shape->miterLimit);
        
        switch (shape->strokeLineCap)
        {
          case NSVG_CAP_BUTT:   cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);  break;
          case NSVG_CAP_ROUND:  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);   break;
          case NSVG_CAP_SQUARE: cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);  break;
        }
        
        switch (shape->strokeLineJoin)
        {
          case NSVG_JOIN_MITER:   cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);  break;
          case NSVG_JOIN_ROUND:   cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);   break;
          case NSVG_JOIN_BEVEL:   cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);  break;
        }
        
        double dashArray[8];
        
        for (int i = 0; i < shape->strokeDashCount; i++)
          dashArray[i] = shape->strokeDashArray[i];
        
        cairo_set_dash(cr, dashArray, shape->strokeDashCount, shape->strokeDashOffset);
        
        setSource(cr, shape->stroke, shape->opacity);
        cairo_stroke(cr);
      }
    }
  }
}
