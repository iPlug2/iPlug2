#pragma once

#include "IControl.h"
#include "cairo/cairo.h"

#ifdef OS_OSX
#include "cairo/cairo-quartz.h"
#endif

class IGraphicsCairo : public IGraphics
{
public:
  IGraphicsCairo(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsCairo();

  void PrepDraw() override;
  void ReScale() override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa) override;
  void ForcePixel(const IColor& color, int x, int y) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa) override;
  void DrawRect(const IColor& color, const IRECT& rect) override;
  void DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend = nullptr) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle,  const IChannelBlend* pBlend, bool aa) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IChannelBlend* pBlend, bool aa) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  void FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa) override;
  void FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend) override;

  void FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  void FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend) override;
  void FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return (void*) mContext; }

  bool DrawIText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureIText(const IText& text, const char* str, IRECT& destRect) override;
  
  IBitmap LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale) override;
  IBitmap CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale) override;
  void ReleaseIBitmap(IBitmap& bitmap) override;
  void RetainIBitmap(IBitmap& bitmap, const char * cacheName) override;
  
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
  
public:
  inline float CairoWeight(const IChannelBlend* pBlend)
  {
    return (pBlend ? pBlend->mWeight : 1.0f);
  }
  
  inline cairo_operator_t CairoBlendMode(const IChannelBlend* pBlend)
  {
    if (!pBlend)
    {
      return CAIRO_OPERATOR_OVER;
    }
    switch (pBlend->mMethod)
    {
      case IChannelBlend::kBlendClobber:
      {
        return CAIRO_OPERATOR_OVER;
      }
      case IChannelBlend::kBlendAdd:
      {
        return CAIRO_OPERATOR_ADD;
      }
      case IChannelBlend::kBlendColorDodge:
      {
        return CAIRO_OPERATOR_COLOR_DODGE;
      }
      case IChannelBlend::kBlendNone:
      default:
      {
        return CAIRO_OPERATOR_OVER; // TODO: is this correct - same as clobber?
      }
    }
  }
  
  inline void SetCairoSourceRGBA(const IColor& color, const IChannelBlend* pBlend = nullptr)
  {
    cairo_set_source_rgba(mContext, color.R / 255.0, color.G / 255.0, color.B / 255.0, (CairoWeight(pBlend) * color.A) / 255.0);
  }
  
  inline void CairoDrawRect(const IRECT& rect)
  {
    cairo_rectangle (mContext, rect.L, rect.T, rect.W(), rect.H());
  }
protected:
  cairo_t *mContext;
  cairo_surface_t *mSurface;
};
