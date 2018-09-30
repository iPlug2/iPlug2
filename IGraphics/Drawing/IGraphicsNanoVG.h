#pragma once

#include "IPlugPlatform.h"
#include "IGraphicsPathBase.h"

#include "nanovg.h"

// Thanks to Olli Wang for much of this macro magic  https://github.com/ollix/moui

//#if !defined IGRAPHICS_GL && !defined IGRAPHICS_METAL
//  #if defined OS_MAC || defined OS_IOS
//    #define IGRAPHICS_METAL
//  #elif defined OS_WIN
//    #define IGRAPHICS_GL
//    #define IGRAPHICS_GL2
//  #elif defined OS_WIN
//    #error NOT IMPLEMENTED
//  #elif defined OS_WEB
//    #define IGRAPHICS_GL
//    #define IGRAPHICS_GLES2
//  #endif
//#endif

#ifdef IGRAPHICS_GL
  #if defined IGRAPHICS_GLES2
    #if defined OS_IOS
      #include <OpenGLES/ES2/gl.h>
    #elif defined OS_WEB
      #include <GLES2/gl2.h>
    #endif
  #elif defined IGRAPHICS_GLES3
    #if defined OS_IOS
      #include <OpenGLES/ES3/gl.h>
    #elif defined OS_WEB
      #include <GLES3/gl3.h>
    #endif
  #elif defined IGRAPHICS_GL2
    #include <OpenGL/gl.h>
  #endif
  #include "nanovg_gl_utils.h"
#elif defined IGRAPHICS_METAL
  #include "nanovg_mtl.h"
#else
  #error you must define either IGRAPHICS_GL or IGRAPHICS_METAL when using IGRAPHICS_NANOVG
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

#ifdef IGRAPHICS_GL
  #define nvgBindFramebuffer(fb) nvgluBindFramebuffer(fb)
  #define nvgCreateFramebuffer(ctx, w, h, flags) nvgluCreateFramebuffer(ctx, w, h, flags)
  #define nvgDeleteFramebuffer(fb) nvgluDeleteFramebuffer(fb)
#endif

#if defined IGRAPHICS_GL
typedef NVGLUframebuffer NVGframebuffer;
#elif defined IGRAPHICS_METAL
typedef MNVGframebuffer NVGframebuffer;
#endif

class NanoVGBitmap : public APIBitmap
{
public:
  NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale, void* hInst = nullptr);
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

  IGraphicsNanoVG(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsNanoVG();

  void BeginFrame() override;
  void EndFrame() override;
  void OnViewInitialized(void* pContext) override;
  void OnViewDestroyed() override;
  void DrawResize() override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { nvgBeginPath(mVG); }
  void PathClose() override { nvgClosePath(mVG); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { nvgArc(mVG, cx, cy, r, DegToRad(aMin - 90.f), DegToRad(aMax - 90.f), NVG_CW);}

  void PathMoveTo(float x, float y) override { nvgMoveTo(mVG, x, y); }
  void PathLineTo(float x, float y) override { nvgLineTo(mVG, x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { nvgBezierTo(mVG, x1, y1, x2, y2, x3, y3); }
    
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
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
  
  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
  
  StaticStorage<APIBitmap> mBitmapCache; //not actually static
  NVGcontext* mVG = nullptr;
  NVGframebuffer* mMainFrameBuffer = nullptr;
#if defined OS_WIN
  HGLRC mHGLRC = nullptr;
#endif
};
