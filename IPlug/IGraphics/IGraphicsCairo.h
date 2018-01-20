#pragma once

#include "IPlugOSDetect.h"

#include "cairo/cairo.h"
#ifdef OS_OSX
#include "cairo/cairo-quartz.h"
#else
#include "cairo/cairo-win32.h"
#endif

#ifndef NO_FREETYPE
#include "cairo/cairo-ft.h"
#endif

#include "IGraphics.h"

/** IGraphics draw class using Cairo  
*   @ingroup DrawClasses
*/
class IGraphicsCairo : public IGraphics
{
public:
  const char* GetDrawingAPIStr() override { return "CAIRO"; }

  IGraphicsCairo(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsCairo();

  void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend) override;
  void DrawRotatedSVG(ISVG& bitmap, float destCtrX, float destCtrY, double angle, float yOffsetZeroDeg, const IBlend* pBlend) override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend, bool aa) override;
  void ForcePixel(const IColor& color, int x, int y) override;
  
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, bool aa) override;
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle,  const IBlend* pBlend, bool aa) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend, bool aa) override;
  void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, float cr, bool aa) override;
  void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;

  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, bool aa) override;
  void FillIRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, float cr, bool aa) override;
  void FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IBlend* pBlend) override;
  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return (void*) mContext; }

  bool DrawIText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureIText(const IText& text, const char* str, IRECT& destRect) override;
  
  IBitmap LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale) override;
  IBitmap CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale) override;
  void ReleaseIBitmap(IBitmap& bitmap) override;
  void RetainIBitmap(IBitmap& bitmap, const char * cacheName) override;
  
  void RenderDrawBitmap() override;
  
  void SetPlatformContext(void* pContext) override;
  
  inline void ClipRegion(const IRECT& r) override
  {
    cairo_new_path(mContext);
    CairoDrawRect(r);
    cairo_clip(mContext);
  }
  
  inline void ResetClipRegion() override
  {
    cairo_reset_clip(mContext);
  }
  
protected:
  inline float CairoWeight(const IBlend* pBlend)
  {
    return (pBlend ? pBlend->mWeight : 1.0f);
  }
  
  inline cairo_operator_t CairoBlendMode(const IBlend* pBlend)
  {
    if (!pBlend)
    {
      return CAIRO_OPERATOR_OVER;
    }
    switch (pBlend->mMethod)
    {
      case IBlend::kBlendClobber:
      {
        return CAIRO_OPERATOR_OVER;
      }
      case IBlend::kBlendAdd:
      {
        return CAIRO_OPERATOR_ADD;
      }
      case IBlend::kBlendColorDodge:
      {
        return CAIRO_OPERATOR_COLOR_DODGE;
      }
      case IBlend::kBlendNone:
      default:
      {
        return CAIRO_OPERATOR_OVER; // TODO: is this correct - same as clobber?
      }
    }
  }
  
  inline void SetCairoSourceRGBA(const IColor& color, const IBlend* pBlend = nullptr)
  {
    cairo_set_operator(mContext, CairoBlendMode(pBlend));
    cairo_set_source_rgba(mContext, color.R / 255.0, color.G / 255.0, color.B / 255.0, (CairoWeight(pBlend) * color.A) / 255.0);
  }
  
  inline void CairoDrawRect(const IRECT& rect)
  {
    cairo_rectangle(mContext, rect.L, rect.T, rect.W(), rect.H());
  }

private:
  cairo_t* mContext;
  cairo_surface_t* mSurface;
};
