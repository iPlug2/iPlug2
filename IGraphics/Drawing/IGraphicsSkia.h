#pragma once

#include "IPlugPlatform.h"
#include "IGraphicsPathBase.h"

#include "SkSurface.h"
#include "SkPath.h"
#include "SkCanvas.h"
#include "SkBitmap.h"

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

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { mMainPath.reset(); }
  void PathClose() override { mMainPath.close(); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { /*TODO;*/ }

  void PathMoveTo(float x, float y) override { mMainPath.moveTo(x, y); }
  void PathLineTo(float x, float y) override { mMainPath.lineTo(x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { /*TODO;*/ }
    
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return (void*) mSurface->getCanvas(); }

  bool DrawText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override;
  
  IBitmap ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale) override { return bitmap; } // NO-OP
  void ReleaseBitmap(const IBitmap& bitmap) override { }; // NO-OP
  void RetainBitmap(const IBitmap& bitmap, const char * cacheName) override { }; // NO-OP

  void LoadFont(const char* name) override;
  
  void DrawBoxShadow(const IRECT& bounds, float cr, float ydrop, float pad, const IBlend* pBlend) override;
  void SetPlatformContext(void* pContext) override;
protected:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override { return new APIBitmap(); } // NO-OP
private:
  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
  sk_sp<SkSurface> mSurface;
  SkPath mMainPath;
};
