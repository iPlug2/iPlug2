#pragma once

#include "IPlugPlatform.h"

#include "nanovg.h"
#if defined OS_MAC || defined OS_IOS
#include "nanovg_mtl.h"
#endif

#include "IGraphicsPathBase.h"

class PerfGraph;

class NanoVGBitmap : public APIBitmap
{
public:
  NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale);
  virtual ~NanoVGBitmap();
private:
  NVGcontext* mVG;
};

/** IGraphics draw class using NanoVG  
*   @ingroup DrawClasses
*/
class IGraphicsNanoVG : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override { return "NANOVG"; }

  IGraphicsNanoVG(IEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsNanoVG();

  void BeginFrame() override;
  void EndFrame() override;
  void OnViewInitialized(void* pContext) override;
  void OnViewDestroyed() override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { } //TODO:?
  void PathStart() override { nvgBeginPath(mVG); }
  void PathClose() override { nvgClosePath(mVG); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { nvgArc(mVG, cx, cy, r, DegToRad(aMin), DegToRad(aMax), NVG_CW);}

  void PathMoveTo(float x, float y) override { nvgMoveTo(mVG, x, y); }
  void PathLineTo(float x, float y) override { nvgLineTo(mVG, x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { nvgBezierTo(mVG, x1, y1, x2, y2, x3, y3); }
    
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  void PathStateSave() override { nvgSave(mVG); }
  void PathStateRestore() override { nvgRestore(mVG); }
    
  void PathTransformTranslate(float x, float y) override { nvgTranslate(mVG, x, y); }
  void PathTransformScale(float scaleX, float scaleY) override { nvgScale(mVG, scaleX, scaleY); }
  void PathTransformRotate(float angle) override { nvgRotate(mVG, DegToRad(angle)); }
    
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return (void*) mVG; }

  bool DrawText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override;
  
  IBitmap LoadBitmap(const char* name, int nStates, bool framesAreHorizontal) override;
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
  
  void ClipRegion(const IRECT& r) override { nvgScissor(mVG, r.L, r.T, r.W(), r.H()); }
  void ResetClipRegion() override { nvgResetScissor(mVG); }

  StaticStorage<APIBitmap> mBitmapCache; //not actually static
  NVGcontext* mVG = nullptr;
#ifdef OS_WIN
  HGLRC mHGLRC = nullptr;
#endif
#if NANOVG_PERF
  PerfGraph* mPerfGraph = nullptr;
  double mPrevTimestamp = 0.;
#endif
};
