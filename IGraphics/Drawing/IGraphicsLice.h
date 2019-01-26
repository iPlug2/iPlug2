/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"

#ifdef OS_MAC
  #include <CoreGraphics/CoreGraphics.h>
  #include "swell.h"
#elif defined OS_WIN
  #pragma comment(lib, "libpng.lib")
  #pragma comment(lib, "zlib.lib")
#elif defined OS_LINUX
  #include "swell.h"
#else
  #error NOT IMPLEMENTED
#endif

#include "IGraphicsLice_src.h"
#include "IGraphics.h"

inline LICE_pixel LiceColor(const IColor& color)
{
  auto preMul = [](int color, int A) {return (color * (A + 1)) >> 8; };
  return LICE_RGBA(preMul(color.R, color.A), preMul(color.G, color.A), preMul(color.B, color.A), color.A);
}

inline LICE_pixel LiceColor(const IColor& color, const IBlend* pBlend)
{
    int alpha = std::round(color.A * BlendWeight(pBlend));
    return LICE_RGBA(color.R, color.G, color.B, alpha);
}

inline int LiceBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
  }
  switch (pBlend->mMethod)
  {
    case EBlendType::kBlendClobber:     return LICE_BLIT_MODE_COPY;
    case EBlendType::kBlendAdd:         return LICE_BLIT_MODE_ADD | LICE_BLIT_USE_ALPHA;
    case EBlendType::kBlendDefault:
    default:
    {
      return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
    }
  }
}

/** A LICE API bitmap
 * @ingroup APIBitmaps */
class LICEBitmap : public APIBitmap
{
public:
  LICEBitmap(LICE_IBitmap* pBitmap, int scale, bool preMultiplied) : APIBitmap (pBitmap, pBitmap->getWidth(), pBitmap->getHeight(), scale, 1.f), mPremultiplied(preMultiplied) {}
  virtual ~LICEBitmap() { delete ((LICE_IBitmap*) GetBitmap()); }
  
  bool IsPreMultiplied() { return mPremultiplied; }
    
private:
  bool mPremultiplied;
};

/** IGraphics draw class using Cockos' LICE  
*   @ingroup DrawClasses */
class IGraphicsLice : public IGraphics
{
public:
  const char* GetDrawingAPIStr() override { return "LICE"; }

  IGraphicsLice(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsLice();

  void DrawResize() override;

  void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend) override;
  void DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend) override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, float destCtrX, float destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, float x, float y, double angle, const IBlend* pBlend) override;
  void DrawFittedBitmap(IBitmap& bitmap, const IRECT& bounds, const IBlend* pBlend) override;
  
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness) override;
  void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen) override;
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend, float thickness) override;
  void DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness) override;
  void DrawRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend, float thickness) override;
  void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend, float thickness) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend, float thickness) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend, float thickness) override;
  void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen) override;

  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend) override;
  void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override;
  void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend) override;
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override;
    
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return mDrawBitmap->getBits(); }
  inline LICE_SysBitmap* GetDrawBitmap() const { return mDrawBitmap; }

  // Not implemented
  void DrawRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend, float thickness) override { /* TODO - mark unsupported */ }
  void DrawEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness) override { /* TODO - mark unsupported */ }
  void DrawEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend, float thickness) override { /* TODO - mark unsupported */ }
  void FillRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend) override { /* TODO - mark unsupported */ }
  void FillEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend) override { /* TODO - mark unsupported */ }
  void FillEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend) override { /* TODO - mark unsupported */ }

  bool BitmapExtSupported(const char* ext) override;
protected:
  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;
  APIBitmap* CreateAPIBitmap(int width, int height) override;

  int AlphaChannel() const override { return LICE_PIXEL_A; }
  bool FlippedBitmap() const override { return false; }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  bool DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure) override;

  void EndFrame() override;
    
  float GetBackingPixelScale() const override { return (float) GetScreenScale(); };

private:
    
  bool OpacityCheck(const IColor& color, const IBlend* pBlend)
  {
    return (color.A == 255) && BlendWeight(pBlend) >= 1.f;
  }
    
  template<typename T, typename... Args>
  void OpacityLayer(T method, const IBlend* pBlend, const IColor& color, Args... args);
    
  float TransformX(float x)
  {
    return (x - mDrawOffsetX) * GetScreenScale();
  }
  
  float TransformY(float y)
  {
    return (y - mDrawOffsetY) * GetScreenScale();
  }
    
  IRECT TransformRECT(const IRECT& r)
  {
    IRECT tr = r;
    tr.Translate(-mDrawOffsetX, -mDrawOffsetY);
    tr.Scale(GetScreenScale());
    return tr;
  }
    
  void NeedsClipping();
  void PrepareRegion(const IRECT& r) override;
  void CompleteRegion(const IRECT& r) override;
    
  void UpdateLayer() override;
    
  LICE_IFont* CacheFont(const IText& text, double scale);

  IRECT mDrawRECT;
  IRECT mClipRECT;
    
  int mDrawOffsetX = 0;
  int mDrawOffsetY = 0;
  
  LICE_SysBitmap* mDrawBitmap = nullptr;
  LICE_MemBitmap* mTmpBitmap = nullptr;
#ifdef OS_WIN
  LICE_SysBitmap* mScaleBitmap = nullptr;
#endif
  // N.B. mRenderBitmap is not owned through this pointer, and should not be deleted
  LICE_IBitmap* mRenderBitmap = nullptr;
    
  ILayerPtr mClippingLayer;
    
#ifdef OS_MAC
  CGColorSpaceRef mColorSpace = nullptr;
#endif
};
