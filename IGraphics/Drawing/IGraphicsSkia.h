#pragma once

#include "IPlugPlatform.h"
#include "IGraphicsPathBase.h"

#include "SkSurface.h"
#include "SkPath.h"
#include "SkCanvas.h"
#include "SkImage.h"

#ifdef IGRAPHICS_GL2
#define IGRAPHICS_GL
#elif defined IGRAPHICS_CPU
//todo
#else
#error you must define either IGRAPHICS_GL2, IGRAPHICS_GL3 or IGRAPHICS_CPU when using IGRAPHICS_SKIA
#endif


class SkiaBitmap : public APIBitmap
{
public:
  SkiaBitmap(const char* path, double sourceScale);
private:
  sk_sp<SkImage> mImage;
};

/** IGraphics draw class using Skia
*   @ingroup DrawClasses */
class IGraphicsSkia : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override { return "SKIA"; }

  IGraphicsSkia(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsSkia();

  void BeginFrame() override;
  void EndFrame() override;
  void OnViewInitialized(void* pContext) override;
  void OnViewDestroyed() override;
  void DrawResize() override;

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { mMainPath.reset(); }
  void PathClose() override { mMainPath.close(); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { mMainPath.arcTo(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r), aMin - 90.f, (aMax - aMin) - 0.01f /* TODO: ? */, false); }

  void PathMoveTo(float x, float y) override { mMainPath.moveTo(x, y); }
  void PathLineTo(float x, float y) override { mMainPath.lineTo(x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { mMainPath.cubicTo({x1, y1}, {x2, y2}, {x3, y3}); }
    
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return (void*) mCanvas; }

  bool BitmapExtSupported(const char* ext) override;
  int AlphaChannel() const override { return 0; } // TODO:
  bool FlippedBitmap() const override { return false; }

  IBitmap ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale) override { return bitmap; } // NO-OP
  void ReleaseBitmap(const IBitmap& bitmap) override { }; // NO-OP
  void RetainBitmap(const IBitmap& bitmap, const char * cacheName) override { }; // NO-OP
  APIBitmap* CreateAPIBitmap(int width, int height, int scale, double drawScale) override { return nullptr; }; // TODO:

  void SetPlatformContext(void* pContext) override;
  
  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override {}; // TODO:
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override {}; // TODO:

protected:
  void DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override { return false; } // TODO:

  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
private:
  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
  sk_sp<SkSurface> mSurface;
  SkCanvas* mCanvas = nullptr;
  sk_sp<GrContext> mGrContext;
  std::unique_ptr<GrBackendRenderTarget> mBackendRenderTarget;
  SkPath mMainPath;
};
