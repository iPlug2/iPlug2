/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 D2D backend originally contributed by Joseph Broms

 ==============================================================================
*/


#pragma once

#include <map>

#include "IPlugPlatform.h"

#ifndef OS_WIN
#error Direct2D backend only works on Windows!
#endif

#include <d2d1_1.h>
//#include <d2d1helper.h>
#include <d3d11_1.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <wincodec.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "d3d11.lib")

#include "IGraphicsPathBase.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics draw class using Direct2D
*   @ingroup DrawClasses */
class IGraphicsD2D : public IGraphicsPathBase
{
private:
  class Bitmap;
  class Font;
  struct OSFont;
public:
  IGraphicsD2D(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsD2D();

  const char* GetDrawingAPIStr() override { return "DIRECT2D"; }

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  // basic path construction
  void PathClear() override;
  void PathClose() override;
  void PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding = EWinding::CW) override;
  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;
  void PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2) override;
  void PathQuadraticBezierTo(float cx, float cy, float x2, float y2) override;
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;

  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return (void*) mD2DDeviceContext; }

  void BeginFrame() override;
  void EndFrame() override;
  void SetPlatformContext(void* pContext) override;
  void DrawResize() override;

  bool BitmapExtSupported(const char* ext) override;

  // shape drawing overrides.  IGraphics can handle the follow methods using basic path construction
  // but Direct2D provides optimized versions which will tessellate and fill much faster in certain cases.
  //void DrawRotatedBitmap(const IBitmap& bitmap, float destCtrX, float destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  //void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness) override;
  //void DrawGrid(const IColor& color, const IRECT& bounds, float gridSizeH, float gridSizeV, const IBlend* pBlend, float thickness) override;
  //void DrawData(const IColor& color, const IRECT& bounds, float* normYPoints, int nPoints, float* normXPoints, const IBlend* pBlend, float thickness) override;
  //void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen) override;
  //void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend, float thickness) override;
  void DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness) override;
  void DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend, float thickness) override;
  //void DrawRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend, float thickness) override;
  //void DrawConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend, float thickness) override;
  //void DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend, float thickness) override;
  //void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, float thickness) override;
  //void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen) override;
  //void DrawEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness) override;
  //void DrawEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend, float thickness) override;
  //void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend) override;
  //void FillRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend) override;
  //void FillConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend) override;
  //void FillArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend) override;
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override;
  void FillEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend) override;
  void FillEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend) override;
  //void PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3) override;
  //void PathRect(const IRECT& bounds) override;
  //void PathRoundRect(const IRECT& bounds, float ctl, float ctr, float cbl, float cbr) override;
  //void PathRoundRect(const IRECT& bounds, float cr) override;
  //void PathEllipse(float x, float y, float r1, float r2, float angle = 0.0) override;
  //void PathEllipse(const IRECT& bounds) override;
  //void PathCircle(float cx, float cy, float r) override;

protected:
  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
  APIBitmap* CreateAPIBitmap(int width, int height, int scale, double drawScale) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override;

  int AlphaChannel() const override { return 3; }
  bool FlippedBitmap() const override { return false; }
  bool HasPathSupport() const override { return true; }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  void DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

private:

  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
  void UpdateLayer() override  {  DBGMSG("Not yet supported"); }

  static StaticStorage<Font> sFontCache;

  // starts up Direct2D -- this is a one time setup for the window passed in.
  void D2DInitialize();

  // cleans up everything for Direct2D
  void D2DFinalize();

  HRESULT D2DCreateDeviceHelper(D3D_DRIVER_TYPE const type, ID3D11Device*& device);
  void D2DCreateDevice();
  void D2DReleaseDevice();
  void D2DCreateDeviceSwapChainBitmap();
  void D2DResizeSurface();
  void D2DCreateFactoryResources();
  void D2DReleaseFactoryResources();
  void D2DReleaseDeviceDependantResources();
  void D2DReleaseSizeDependantResources();

  ID2D1Factory1* mFactory = nullptr;
  IDWriteFactory1* mDWriteFactory = nullptr;
  ID3D11Device* mD3DDevice = nullptr;
  ID2D1Device* mD2DDevice = nullptr;
  IDXGISwapChain1* mSwapChain = nullptr;
  ID2D1DeviceContext* mD2DDeviceContext = nullptr;

  bool mInDraw = false;
  UINT mLastVsync = 0;
  bool mPushClipCalled = false;

  // bitmap frame size
  D2D1_SIZE_F mTargetSize;

  // main path support -- IGraphicsD2D supports the one-at-a-time geometry drawing methods
  // in IGraphics.  Its not the most efficient way to draw in Direct2D but it works.  We will
  // keep open a geometry sink too at times.
  ID2D1PathGeometry* mPath = nullptr;
  ID2D1GeometrySink* mPathSink = nullptr;

  // Keep a single color brush 
  ID2D1SolidColorBrush* mSolidBrush = nullptr;

  // brush creation support.  Will return a cached brush if possible.
  ID2D1Brush* GetBrush(const IColor& color);

  // adds a series of points into a path in one call to Direct2D.
  // the array is packed floats of alternating x, y values.
  void PathAddLines(float* points);

  // helper method to load a font
  void PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double& y, IDWriteTextFormat*& font) const;

  class FontCacheItem
  {
  public:
    FontCacheItem(std::string k, IDWriteTextFormat* f) { Key = k; Format = f; Sequence = 0; }
    ~FontCacheItem() { if (Format) Format->Release(); }
    IDWriteTextFormat* Format;
    int Sequence;
    std::string Key;

    static bool SortSeq(const FontCacheItem* l, const FontCacheItem* r) { return l->Sequence < r->Sequence; }
  };

  // returns a hashable name for a font
  std::string FontId(const char* fontName, float fontSize);

  // gets the font from the font cache, or creates a new font and
  // adds it to the cache.  Will return the font pointer.
  IDWriteTextFormat* GetFont(const char* fontName, float fontSize);

  // walks all the fonts and removes all those older than max age.
  // doing the collection can also bump the age of the font by one.
  void GarbageCollectFontCache(int maxItems);

  // removes all fonts
  void NukeFontCache();

  std::map<std::string, FontCacheItem*> mFontCache;
  int mFontSequence = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

