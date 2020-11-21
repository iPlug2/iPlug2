/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <emscripten/val.h>
#include <emscripten/bind.h>

#include "IPlugPlatform.h"

#include "IGraphics.h"

using namespace emscripten;

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Convert IColor to std::string for Canvas */
std::string CanvasColor(const IColor& color, float alpha = 1.0);

/** IGraphics draw class HTML5 canvas
* @ingroup DrawClasses */
class IGraphicsCanvas : public IGraphics
{
private:
  class Bitmap;
  struct Font;
  
public:
  IGraphicsCanvas(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsCanvas();

  const char* GetDrawingAPIStr() override { return "Canvas2D"; }

  void DrawBitmap(const IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend) override;

  void DrawResize() override {};

  void PathClear() override;
  void PathClose() override;
  void PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding) override;
  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;
  void PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2) override;
  void PathQuadraticBezierTo(float cx, float cy, float x2, float y2) override;
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;

  IColor GetPoint(int x, int y) override { return COLOR_BLACK; } // TODO:
  void* GetDrawContext() override { return nullptr; }

  bool BitmapExtSupported(const char* ext) override;
    
protected:
  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
  APIBitmap* LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale) override;
  APIBitmap* CreateAPIBitmap(int width, int height, int scale, double drawScale, bool cacheable = false) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override;

  bool AssetsLoaded() override;

  int AlphaChannel() const override { return 3; }
  bool FlippedBitmap() const override { return false; }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  float DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;
    
private:
  void PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y) const;
    
  val GetContext() const
  {
    val canvas = mLayers.empty() ? val::global("document").call<val>("getElementById", std::string("canvas")) : *(mLayers.top()->GetAPIBitmap()->GetBitmap());
      
    return canvas.call<val>("getContext", std::string("2d"));
  }
    
  void GetFontMetrics(const char* font, const char* style, double& ascenderRatio, double& EMRatio);
  bool CompareFontMetrics(const char* style, const char* font1, const char* font2);
  bool FontExists(const char* font, const char* style);
    
  double XTranslate()  { return mLayers.empty() ? 0 : -mLayers.top()->Bounds().L; }
  double YTranslate()  { return mLayers.empty() ? 0 : -mLayers.top()->Bounds().T; }

  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
    
  void SetCanvasSourcePattern(val& context, const IPattern& pattern, const IBlend* pBlend = nullptr);
  void SetCanvasBlendMode(val& context, const IBlend* pBlend);
    
  std::vector<val> mLoadingFonts;

  static StaticStorage<Font> sFontCache;
};

END_IPLUG_NAMESPACE
END_IGRAPHICS_NAMESPACE

