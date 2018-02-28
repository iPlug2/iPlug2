#pragma once

#include "IPlugPlatform.h"

#include "cairo/cairo.h"
#ifdef OS_MAC
#include "cairo/cairo-quartz.h"
#else
#include "cairo/cairo-win32.h"
#endif

#ifdef IGRAPHICS_FREETYPE
#include "cairo/cairo-ft.h"
#endif

#include "IGraphics.h"

class CairoBitmap : public APIBitmap
{
public:
  CairoBitmap(cairo_surface_t* s, int scale);
  virtual ~CairoBitmap();
};

/** IGraphics draw class using Cairo
*   @ingroup DrawClasses
*/
class IGraphicsCairo : public IGraphics
{
public:
  const char* GetDrawingAPIStr() override { return "CAIRO"; }

  IGraphicsCairo(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsCairo();

  void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend) override;
  void DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend) override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override;
  void ForcePixel(const IColor& color, int x, int y) override;

  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend) override;
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend) override;
  void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend) override;

  void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;

  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend) override;
  void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override;
  void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend) override;
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override;

  bool HasPathSupport() const override { return true; }
    
  void PathStart() override { cairo_new_path(mContext); }
  void PathClose() override { cairo_close_path(mContext); }

  void PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3) override { CairoDrawTriangle(x1, y1, x2, y2, x3, y3); }
  void PathRect(const IRECT& rect) override { CairoDrawRect(rect);}
  void PathRoundRect(const IRECT& rect, float cr = 5.f) override { CairoDrawRoundRect(rect, cr); }
  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { cairo_arc(mContext, cx, cy, r, DegToRad(aMin), DegToRad(aMax)); }
  void PathCircle(float cx, float cy, float r) override { CairoDrawCircle(cx, cy, r); }
  void PathConvexPolygon(float* x, float* y, int npoints) override { CairoDrawConvexPolygon(x, y, npoints); }
    
  void PathMoveTo(float x, float y) override { cairo_move_to(mContext, x, y); }
  void PathLineTo(float x, float y) override { cairo_line_to(mContext, x, y);}
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { cairo_curve_to(mContext, x1, y1, x2, y2, x3, y3); }

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return (void*) mContext; }

  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;

  //IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, int targetScale) override;

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

  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;

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
      case kBlendClobber: return CAIRO_OPERATOR_OVER;
      case kBlendAdd: return CAIRO_OPERATOR_ADD;
      case kBlendColorDodge: return CAIRO_OPERATOR_COLOR_DODGE;
      case kBlendNone:
      default:
        return CAIRO_OPERATOR_OVER; // TODO: is this correct - same as clobber?
    }
  }
  
  void SetCairoSourcePattern(const IPattern& pattern, const IBlend* pBlend);
    
  void Stroke(const IPattern& pattern, const IBlend* pBlend)
  {
    SetCairoSourcePattern(pattern, pBlend);
    cairo_set_line_width(mContext, 1);
    cairo_stroke(mContext);
  }
  
  void Fill(const IPattern& pattern, const IBlend* pBlend)
  {
    SetCairoSourcePattern(pattern, pBlend);
    cairo_fill(mContext);
  }

  inline void CairoDrawRect(const IRECT& rect)
  {
    cairo_rectangle(mContext, rect.L, rect.T, rect.W(), rect.H());
  }

  void CairoDrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
  void CairoDrawRoundRect(const IRECT& rect, float corner);
  void CairoDrawConvexPolygon(float* x, float* y, int npoints);
  void CairoDrawCircle(float cx, float cy, float r);

  void CairoSetStrokeOptions(const IStrokeOptions& options = IStrokeOptions());
  void CairoSetFillOptions(const IFillOptions& options = IFillOptions());
  
private:
  cairo_t* mContext;
  cairo_surface_t* mSurface;
};
