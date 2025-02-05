/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "IGraphics.h"

#include "visage/visage_graphics/canvas.h"
#include "visage/visage_graphics/color.h"
#include "visage/visage_graphics/shapes.h"

#include <stack>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics draw class using Visage  
*   @ingroup DrawClasses */
class IGraphicsVisage : public IGraphics
{
private:
  class Bitmap;
  
public:
  IGraphicsVisage(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsVisage();

  const char* GetDrawingAPIStr() override;

  void BeginFrame() override;
  void EndFrame() override;
  void OnViewInitialized(void* pContext) override;
  void OnViewDestroyed() override;
  void DrawResize() override;

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = nullptr, float thickness = 1.f) override;
  void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = nullptr, float thickness = 1.f, float dashLen = 2.f) override;
  void DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = nullptr, float thickness = 1.f) override;
  void DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius = 5.f, const IBlend* pBlend = nullptr, float thickness = 1.f) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = nullptr, float thickness = 1.f) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend = nullptr, float thickness = 1.f) override;
  void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = nullptr, float thickness = 1.f, float dashLen = 2.f) override;

  void FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = nullptr) override;
  void FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius = 5.f, const IBlend* pBlend = nullptr) override;
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = nullptr) override;

  void DrawFastDropShadow(const IRECT& innerBounds, const IRECT& outerBounds, float xyDrop = 5.f, float roundness = 0.f, float blur = 10.f, IBlend* pBlend = nullptr) override;
  
  void DrawMultiLineText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;
  
  void PathClear() override;
  void PathClose() override;
  void PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding) override;
  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;
  void PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2) override;
  void PathQuadraticBezierTo(float cx, float cy, float x2, float y2) override;
  void PathSetWinding(bool clockwise) override;
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return nullptr; }
    
  IBitmap LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale) override;
  void ReleaseBitmap(const IBitmap& bitmap) override;
  void RetainBitmap(const IBitmap& bitmap, const char* cacheName) override;
  bool BitmapExtSupported(const char* ext) override;
  
protected:
  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
  APIBitmap* LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale) override;
  APIBitmap* CreateAPIBitmap(int width, int height, float scale, double drawScale, bool cacheable = false) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override;

  int AlphaChannel() const override { return 3; }
  bool FlippedBitmap() const override { return false; }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  float DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

private:
  void PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double& y) const;
  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
  void UpdateLayer() override;
  
  int FromIColor(const IColor& color, const IBlend* pBlend = nullptr) const
  {
    return (((int)(color.A * (pBlend ? pBlend->mWeight : 1.f)) << 24) | 
            (color.R << 16) | 
            (color.G << 8) | 
            color.B);
  }
  
  visage::Canvas mCanvas;
  StaticStorage<APIBitmap> mBitmapCache;
  
  friend class IGraphicsTest;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
