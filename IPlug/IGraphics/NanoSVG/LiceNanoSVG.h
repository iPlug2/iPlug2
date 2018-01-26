#pragma once

#include "lice.h"
#include "nanosvg.h"

namespace LiceNanoSVGRender
{
  static LICE_pixel GetLiceColor(uint32_t color)
  {
    return LICE_RGBA(0, 0, 0, 0);
//     return LICE_RGBA((color >> 0) & 0xff,
//                      (color >> 8) & 0xff,
//                      (color >> 16) & 0xff,
//                      (color >> 24) & 0xff);
  }
//
  static void SetSource(LICE_IBitmap* pBitmap, const NSVGpaint& paint, float opacity)
  {
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
      {
        return;
      }
      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      {
        return;
      }
      default:
        return;
    }
  }
  
  static void RenderNanoSVG(LICE_IBitmap* pBitmap, NSVGimage* pImage)
  {
    for (NSVGshape* pShape = pImage->shapes; pShape; pShape = pShape->next)
    {
      if (!(pShape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
//      LICE_pixel color = GetLiceColor(pShape->fill.color);
//      LICE_DrawText(pBitmap, 0, 0, "no has svg", 0, 1, LICE_BLIT_MODE_COPY);

//      for (NSVGpath* pPath = pShape->paths; pPath; pPath = pPath->next)
//      {
//        for (auto i = 0; i < pPath->npts-1; i += 3) {
//          float *p = pPath->pts + i * 2 + 2;
          //LICE_DrawCBezier(pBitmap, p[0], p[1], p[2], p[3], p[4], p[5], color, 1., LICE_BLIT_MODE_COPY, true);
//        }
//      }
      
      // Fill
      
      if (pShape->fill.type != NSVG_PAINT_NONE)
      {

      }
      
      // Stroke
      
      if (pShape->stroke.type != NSVG_PAINT_NONE)
      {

      }
    }
  }
}

