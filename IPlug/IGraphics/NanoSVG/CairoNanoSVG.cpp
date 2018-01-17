#include "CairoNanoSVG.h"

namespace CairoNanoSVGRender
{
  void setSource(cairo_t* cr, const NSVGpaint& paint, float opacity)
  {
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
      {
        float r = ((paint.color >> 0) & 0xFF) / 255.0;
        float g = ((paint.color >> 8) & 0xFF) / 255.0;
        float b = ((paint.color >> 16) & 0xFF) / 255.0;
        cairo_set_source_rgba(cr, r, g, b, opacity);
        break;
      }
      //case NSVG_PAINT_LINEAR_GRADIENT:
      //case NSVG_PAINT_RADIAL_GRADIENT:
        
      default:
        cairo_set_source_rgba(cr, 0, 0, 0, opacity);
    }
  }
  
  void RenderNanoSVG(cairo_t* cr, NSVGimage* image)
  {
    for (NSVGshape* shape = image->shapes; shape; shape = shape->next)
    {
      if (!(shape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
      //cairo_save(cr);
      //cairo_rectangle(cr, shape->bounds[0], shape->bounds[1], shape->bounds[2] - shape->bounds[0], shape->bounds[3] - shape->bounds[1]);
      //cairo_clip(cr);
      
      for (NSVGpath* path = shape->paths; path; path = path->next)
      {
        //cairo_save(cr);
        //cairo_rectangle(cr, path->bounds[0], path->bounds[1], path->bounds[2] - path->bounds[0], path->bounds[3] - path->bounds[1]);
        //cairo_clip();
        
        cairo_move_to(cr, path->pts[0], path->pts[1]);
        
        for (int i = 0; i < path->npts - 1; i += 3)
        {
          float* p = path->pts + i * 2 + 2;
          cairo_curve_to(cr, p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        
        if (path->closed)
          cairo_close_path(cr);
        //cairo_restore(cr);
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
          case NSVG_CAP_BUTT:  cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);  break;
          case NSVG_CAP_ROUND: cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);  break;
          case NSVG_CAP_SQUARE: cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE); break;
        }
        
        switch (shape->strokeLineJoin)
        {
          case NSVG_JOIN_MITER:  cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);  break;
          case NSVG_JOIN_ROUND:  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);  break;
          case NSVG_JOIN_BEVEL:  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL); break;
        }
        
        double dashArray[8];
        
        for (int i = 0; i < shape->strokeDashCount; i++)
          dashArray[i] = shape->strokeDashArray[i];
        
        cairo_set_dash(cr, dashArray, shape->strokeDashCount, shape->strokeDashOffset);
        
        setSource(cr, shape->stroke, shape->opacity);
        cairo_stroke(cr);
      }
      //cairo_restore(cr);
    }
  }
}
