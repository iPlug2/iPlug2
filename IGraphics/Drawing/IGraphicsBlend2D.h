/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "IGraphics.h"

#include <blend2d.h>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Converts IRECT to a BLRect */
BLRect Blend2DRect(const IRECT& r);

/** Converts IColor to a BLRgba32 */
BLRgba32 Blend2DColor(const IColor& color, const IBlend* pBlend = nullptr);

/** Gets BLCompOp from IBlend */
BLCompOp Blend2DCompOp(const IBlend* pBlend);

/** Gets BLExtendMode from IPattern */
BLExtendMode Blend2DExtendMode(const IPattern& pattern);

/** IGraphics draw class using Blend2D
*   @ingroup DrawClasses */
class IGraphicsBlend2D : public IGraphics
{
private:
  class Bitmap;
  struct Font;
public:
  IGraphicsBlend2D(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsBlend2D();

  const char* GetDrawingAPIStr() override;

  void BeginFrame() override;
  void EndFrame() override;
  void OnViewInitialized(void* pContext) override;
  void OnViewDestroyed() override;
  void DrawResize() override;

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { mMainPath.reset(); }
  void PathClose() override { mMainPath.close(); }
  void PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding) override;

  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;

  void PathCubicBezierTo(float x1, float y1, float x2, float y2, float x3, float y3) override;
  void PathQuadraticBezierTo(float cx, float cy, float x2, float y2) override;

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;

  void DrawFastDropShadow(const IRECT& innerBounds, const IRECT& outerBounds, float xyDrop, float roundness, float blur, IBlend* pBlend) override;

  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return (void*) &mContext; }

  bool BitmapExtSupported(const char* ext) override;
  int AlphaChannel() const override { return 3; }
  bool FlippedBitmap() const override { return false; }

  APIBitmap* CreateAPIBitmap(int width, int height, float scale, double drawScale, bool cacheable = false) override;

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  void UpdateLayer() override;

  void DrawMultiLineText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

protected:

  float DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override;

  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
  APIBitmap* LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale) override;

private:
  void PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y, BLFont& font) const;

  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;

  void SetupContext(BLContext& ctx, BLImage& img);
  BLGradient CreateGradient(const IPattern& pattern, const IBlend* pBlend);

  BLImage mSurface;
  BLContext mContext;
  BLPath mMainPath;
  BLMatrix2D mMatrix;
  BLMatrix2D mClipMatrix;

#if defined OS_WIN
  WDL_TypedBuf<uint8_t> mSurfaceMemory;
#endif

  static StaticStorage<Font> sFontCache;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
