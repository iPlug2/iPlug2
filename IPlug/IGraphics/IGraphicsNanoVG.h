#pragma once

#include "IPlugPlatform.h"

#include "nanovg.h"
#ifdef OS_MAC
#include "nanovg_mtl.h"
#endif

#include "IGraphicsPathBase.h"

class NanoVGBitmap : public APIBitmap
{
public:
  NanoVGBitmap(NVGcontext* context, const char* path, double sourceScale);
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

  IGraphicsNanoVG(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsNanoVG();

  void BeginFrame() override;
  void EndFrame() override;
  void ViewInitialized(void* layer) override;
  
  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { nvgBeginPath(mVG); }
  void PathStart() override { }
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
  void* GetData() override { return (void*) mVG; }

  bool DrawText(const IText& text, const char* str, IRECT& bounds, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override;
  
  IBitmap LoadBitmap(const char* name, int nStates, bool framesAreHorizontal) override;
  IBitmap ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale) override;
//  void ReleaseBitmap(const IBitmap& bitmap) override;
  void RetainBitmap(const IBitmap& bitmap, const char * cacheName) override;
//  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override {}

protected:

  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;

private:
  
  void ClipRegion(const IRECT& r) override { nvgScissor(mVG, r.L, r.T, r.W(), r.H()); }
  void ResetClipRegion() override { nvgResetScissor(mVG); }
  
  WDL_PtrList<NanoVGBitmap> mBitmaps;
  NVGcontext* mVG = nullptr;
};
