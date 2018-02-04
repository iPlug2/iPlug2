#pragma once

#include "IPlugPlatform.h"

#ifdef OS_OSX
#include <CoreGraphics/CoreGraphics.h>
#include "swell.h"
#endif
#include "lice.h"
#include "lice_text.h"

#include "IGraphics.h"

/** IGraphics draw class using Cockos' LICE  
*   @ingroup DrawClasses
*/
class IGraphicsLice : public IGraphics
{
public:
  const char* GetDrawingAPIStr() override { return "LICE"; }

  IGraphicsLice(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsLice();

  void SetDisplayScale(int) override;

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
    
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return mDrawBitmap->getBits(); }

  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;
  
  //IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, int targetScale) override;
  
  inline LICE_SysBitmap* GetDrawBitmap() const { return mDrawBitmap; }

protected:
    
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;

  void RenderDrawBitmap() override;
    
private:
  LICE_IFont* CacheFont(const IText& text, double scale);

  LICE_SysBitmap* mDrawBitmap = nullptr;
  LICE_MemBitmap* mTmpBitmap = nullptr;
#ifdef OS_OSX
  CGColorSpaceRef mColorSpace = nullptr;
#endif
};

inline LICE_pixel LiceColor(const IColor& color)
{
  return LICE_RGBA(color.R, color.G, color.B, color.A);
}

inline float LiceWeight(const IBlend* pBlend)
{
  return (pBlend ? pBlend->mWeight : 1.0f);
}

inline int LiceBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
  }
  switch (pBlend->mMethod)
  {
    case IBlend::kBlendClobber:
    {
      return LICE_BLIT_MODE_COPY;
    }
    case IBlend::kBlendAdd:
    {
      return LICE_BLIT_MODE_ADD | LICE_BLIT_USE_ALPHA;
    }
    case IBlend::kBlendColorDodge:
    {
      return LICE_BLIT_MODE_DODGE | LICE_BLIT_USE_ALPHA;
    }
    case IBlend::kBlendNone:
    default:
    {
      return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
    }
  }
}
