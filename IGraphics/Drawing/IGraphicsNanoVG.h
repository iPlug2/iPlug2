/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "IGraphicsPathBase.h"

#include "nanovg.h"
#include "mutex.h"
#include <stack>

// Thanks to Olli Wang/MOUI for much of this macro magic  https://github.com/ollix/moui

#if defined IGRAPHICS_GL
  #define NANOVG_FBO_VALID 1
  #include "nanovg_gl_utils.h"
#elif defined IGRAPHICS_METAL
  #include "nanovg_mtl.h"
#else
  #error you must define either IGRAPHICS_GL2, IGRAPHICS_GLES2 etc or IGRAPHICS_METAL when using IGRAPHICS_NANOVG
#endif

#if defined IGRAPHICS_GL2
  #define NANOVG_GL2 1
  #define nvgCreateContext(flags) nvgCreateGL2(flags)
  #define nvgDeleteContext(context) nvgDeleteGL2(context)
#elif defined IGRAPHICS_GLES2
  #define NANOVG_GLES2 1
  #define nvgCreateContext(flags) nvgCreateGLES2(flags)
  #define nvgDeleteContext(context) nvgDeleteGLES2(context)
#elif defined IGRAPHICS_GL3
  #define NANOVG_GL3 1
  #define nvgCreateContext(flags) nvgCreateGL3(flags)
  #define nvgDeleteContext(context) nvgDeleteGL3(context)
#elif defined IGRAPHICS_GLES3
  #define NANOVG_GLES3 1
  #define nvgCreateContext(flags) nvgCreateGLES3(flags)
  #define nvgDeleteContext(context) nvgDeleteGLES3(context)
#elif defined IGRAPHICS_METAL
  #define nvgCreateContext(layer, flags) nvgCreateMTL(layer, flags)
  #define nvgDeleteContext(context) nvgDeleteMTL(context)
  #define nvgBindFramebuffer(fb) mnvgBindFramebuffer(fb)
  #define nvgCreateFramebuffer(ctx, w, h, flags) mnvgCreateFramebuffer(ctx, w, h, flags)
  #define nvgDeleteFramebuffer(fb) mnvgDeleteFramebuffer(fb)
#endif

#if defined IGRAPHICS_GL
  #define nvgBindFramebuffer(fb) nvgluBindFramebuffer(fb)
  #define nvgCreateFramebuffer(ctx, w, h, flags) nvgluCreateFramebuffer(ctx, w, h, flags)
  #define nvgDeleteFramebuffer(fb) nvgluDeleteFramebuffer(fb)
  using NVGframebuffer = NVGLUframebuffer;
#elif defined IGRAPHICS_METAL
  using NVGframebuffer = MNVGframebuffer;
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Converts IColor to a NVGcolor */
NVGcolor NanoVGColor(const IColor& color, const IBlend* pBlend = 0);

/** Set the NanoVG context blend based on IBlend */
void NanoVGSetBlendMode(NVGcontext* pContext, const IBlend* pBlend);

/** Converts IPattern to NVGpaint */
NVGpaint NanoVGPaint(NVGcontext* pContext, const IPattern& pattern, const IBlend* pBlend = 0);

/** IGraphics draw class using NanoVG  
*   @ingroup DrawClasses */
class IGraphicsNanoVG : public IGraphicsPathBase
{
private:
  class Bitmap;
  
public:
  IGraphicsNanoVG(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsNanoVG();

  const char* GetDrawingAPIStr() override;

  void BeginFrame() override;
  void EndFrame() override;
  void OnViewInitialized(void* pContext) override;
  void OnViewDestroyed() override;
  void DrawResize() override;

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen) override;
  void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen) override;

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
  void* GetDrawContext() override { return (void*) mVG; }
    
  IBitmap LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale) override;
  void ReleaseBitmap(const IBitmap& bitmap) override { }; // NO-OP
  void RetainBitmap(const IBitmap& bitmap, const char * cacheName) override { }; // NO-OP
  bool BitmapExtSupported(const char* ext) override;

  void DeleteFBO(NVGframebuffer* pBuffer);
    
protected:
  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
  APIBitmap* CreateAPIBitmap(int width, int height, int scale, double drawScale) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override;

  int AlphaChannel() const override { return 3; }
  
  bool FlippedBitmap() const override
  {
#if defined(IGRAPHICS_GL)
    return true;
#else
    return false;
#endif
  }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  float DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

private:
  void PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y) const;
  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
  void UpdateLayer() override;
  void ClearFBOStack();
  
  bool mInDraw = false;
  WDL_Mutex mFBOMutex;
  std::stack<NVGframebuffer*> mFBOStack; // A stack of FBOs that requires freeing at the end of the frame
  StaticStorage<APIBitmap> mBitmapCache; //not actually static (doesn't require retaining or releasing)
  NVGcontext* mVG = nullptr;
  NVGframebuffer* mMainFrameBuffer = nullptr;
  int mInitialFBO = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
