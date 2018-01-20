#pragma once

#include "IPlugOSDetect.h"

#include "nanovg.h"
#ifdef OS_OSX
#include "nanovg_mtl.h"
#endif

#include "IGraphics.h"

struct NanoVGBitmap;

/** IGraphics draw class using NanoVG  
*   @ingroup DrawClasses
*/
class IGraphicsNanoVG : public IGraphics
{
public:
  const char* GetDrawingAPIStr() override { return "NANOVG"; }

  IGraphicsNanoVG(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsNanoVG();

  void BeginFrame() override;
  void EndFrame() override;
  void ViewInitialized(void* layer) override;
  
  void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend) override;
  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override;
  void ForcePixel(const IColor& color, int x, int y) override;
  
  void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle,  const IBlend* pBlend) override;
  void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int cr) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend) override;
  void DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend) override {}

  void FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int cr) override;
  void FillCircle(const IColor& color, int cx, int cy, float r, const IBlend* pBlend) override;
  void FillConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IBlend* pBlend) override;
  void FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return (void*) mVG; }

  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;
  
  IBitmap LoadBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleBitmap(const IBitmap& bitmap, const char* name, double targetScale) override;
  IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale) override;
  void ReleaseBitmap(IBitmap& bitmap) override;
  void RetainBitmap(IBitmap& bitmap, const char * cacheName) override;
//  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override {}

protected:
  WDL_PtrList<NanoVGBitmap> mBitmaps;
  NVGcontext* mVG = nullptr;
};

inline float NanoVGWeight(const IBlend* pBlend)
{
  return (pBlend ? pBlend->mWeight : 1.0f);
}

inline NVGcolor NanoVGColor(const IColor& color, const IBlend* pBlend = 0)
{
  NVGcolor c;
  c.r = (float) color.R / 255.0f;
  c.g = (float) color.G / 255.0f;
  c.b = (float) color.B / 255.0f;
  c.a = (NanoVGWeight(pBlend) * color.A) / 255.0f;
  return c;
}

inline NVGcompositeOperation NanoVGBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    return NVG_COPY;
  }
  switch (pBlend->mMethod)
  {
    case IBlend::kBlendClobber:
    {
      return NVG_SOURCE_OVER;
    }
    case IBlend::kBlendAdd:
//    {
//      return NVG_ATOP;
//    }
    case IBlend::kBlendColorDodge:
//    {
//      return CAIRO_OPERATOR_COLOR_DODGE;
//    }
    case IBlend::kBlendNone:
    default:
    {
      return NVG_COPY;
    }
  }
}
