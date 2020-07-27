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
  #include <IPlugSWELL.h>
#elif defined OS_WIN
  #pragma comment(lib, "libpng.lib")
  #pragma comment(lib, "zlib.lib")
#elif defined OS_LINUX
  #include <IPlugSWELL.h>
#else
  #error NOT IMPLEMENTED
#endif

#include "IGraphicsLice_src.h"
#include "IGraphics.h"

#include <memory>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Converts IColor to a LICE color */
LICE_pixel LiceColor(const IColor& color);

/** Converts IColor to a LICE color, with blend */
LICE_pixel LiceColor(const IColor& color, const IBlend* pBlend);

/** Converts IBlend to LICE blend mode int */
int LiceBlendMode(const IBlend* pBlend);

/** IGraphics draw class using Cockos' LICE  
*   @ingroup DrawClasses */
class IGraphicsLice : public IGraphics
{
private:
  class Bitmap;
  struct FontInfo;
  
public:
  IGraphicsLice(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsLice();

  const char* GetDrawingAPIStr() override { return "LICE"; }

  void DrawResize() override;

  void DrawSVG(const ISVG& svg, const IRECT& dest, const IBlend* pBlend) override;
  void DrawRotatedSVG(const ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend) override;

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(const IBitmap& bitmap, float destCtrX, float destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawFittedBitmap(const IBitmap& bitmap, const IRECT& bounds, const IBlend* pBlend) override;
  
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness) override;
  void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen) override;
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend, float thickness) override;
  void DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness) override;
  void DrawRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend, float thickness) override;
  void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend, float thickness) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2,  const IBlend* pBlend, float thickness) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend, float thickness) override;
  void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen) override;

  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend) override;
  void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override;
  void FillArc(const IColor& color, float cx, float cy, float r, float a1, float a2,  const IBlend* pBlend) override;
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override;
    
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return mDrawBitmap.get(); }

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
  APIBitmap* CreateAPIBitmap(int width, int height, int scale, double drawScale, bool cacheable = false) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override;

  int AlphaChannel() const override { return LICE_PIXEL_A; }
  bool FlippedBitmap() const override { return false; }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  float DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

  void EndFrame() override;
    
  float GetBackingPixelScale() const override { return (float) GetScreenScale(); };

private:
  void PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, LICE_IFont*& pFont) const;
    
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
    
  LICE_IFont* CacheFont(const IText& text) const;

  IRECT mDrawRECT;
  IRECT mClipRECT;
    
  int mDrawOffsetX = 0;
  int mDrawOffsetY = 0;
  
  std::unique_ptr<LICE_SysBitmap> mDrawBitmap;
#ifdef OS_WIN
  std::unique_ptr<LICE_SysBitmap> mScaleBitmap;
#endif
  // N.B. mRenderBitmap is not owned through this pointer, and should not be deleted
  LICE_IBitmap* mRenderBitmap = nullptr;
    
  ILayerPtr mClippingLayer;
  
  static StaticStorage<LICE_IFont> sFontCache;
  static StaticStorage<FontInfo> sFontInfoCache;
    
#ifdef OS_MAC
  class MacRegisteredFont;
  static StaticStorage<MacRegisteredFont> sMacRegistedFontCache;
  CGColorSpaceRef mColorSpace = nullptr;
#endif
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

