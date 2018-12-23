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

// Thanks to Olli Wang for much of this macro magic  https://github.com/ollix/moui

#if defined IGRAPHICS_GLES2
  #define IGRAPHICS_GL
  #if defined OS_IOS
    #include <OpenGLES/ES2/gl.h>
  #elif defined OS_WEB
    #include <GLES2/gl2.h>
  #endif
#elif defined IGRAPHICS_GLES3
  #define IGRAPHICS_GL
  #if defined OS_IOS
    #include <OpenGLES/ES3/gl.h>
  #elif defined OS_WEB
    #include <GLES3/gl3.h>
  #endif
#elif defined IGRAPHICS_GL2 || defined IGRAPHICS_GL3
  #define IGRAPHICS_GL
  #if defined OS_WIN
    #include <glad/glad.h>
  #else
    #include <OpenGL/gl.h>
  #endif
#elif defined IGRAPHICS_METAL
  #include "nanovg_mtl.h"
#else
  #error you must define either IGRAPHICS_GL2, IGRAPHICS_GLES2 etc or IGRAPHICS_METAL when using IGRAPHICS_NANOVG
#endif

#ifdef IGRAPHICS_GL
  #define NANOVG_FBO_VALID 1
  #include "nanovg_gl_utils.h"
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
  typedef NVGLUframebuffer NVGframebuffer;
#elif defined IGRAPHICS_METAL
  typedef MNVGframebuffer NVGframebuffer;
#endif

class NanoVGBitmap : public APIBitmap
{
public:
  NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale, void* hInst = nullptr);
  NanoVGBitmap(NVGcontext* pContext, int width, int height, int scale, float drawScale);
  virtual ~NanoVGBitmap();
  NVGframebuffer* GetFBO() const { return mFBO; }
private:
  NVGcontext* mVG;
  NVGframebuffer* mFBO = nullptr;
};

/** IGraphics draw class using NanoVG  
*   @ingroup DrawClasses
*/

class IGraphicsNanoVG : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override;

  IGraphicsNanoVG(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsNanoVG();

  void BeginFrame() override;
  void EndFrame() override;
  void OnViewInitialized(void* pContext) override;
  void OnViewDestroyed() override;
  void DrawResize() override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen) override;
  void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen) override;

  void PathClear() override;
  void PathClose() override;
  void PathArc(float cx, float cy, float r, float aMin, float aMax) override;
  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override;
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return (void*) mVG; }
    
  IBitmap LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale) override;
  IBitmap ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale) override { return bitmap; } // NO-OP
  void ReleaseBitmap(const IBitmap& bitmap) override { }; // NO-OP
  void RetainBitmap(const IBitmap& bitmap, const char * cacheName) override { }; // NO-OP

  void LoadFont(const char* name) override;
  
  void DrawBoxShadow(const IRECT& bounds, float cr, float ydrop, float pad, const IBlend* pBlend) override;
  void SetPlatformContext(void* pContext) override;

protected:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override { return new APIBitmap(); } // NO-OP
  APIBitmap* CreateAPIBitmap(int width, int height) override;

  bool DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure) override;

private:
  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
  void UpdateLayer() override;

  StaticStorage<APIBitmap> mBitmapCache; //not actually static
  NVGcontext* mVG = nullptr;
  NVGframebuffer* mMainFrameBuffer = nullptr;
    
#if defined OS_WIN
  HGLRC mHGLRC = nullptr;
#endif
};
