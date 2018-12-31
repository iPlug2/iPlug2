/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cmath>

#include "IGraphicsNanoVG.h"
#include "ITextEntryControl.h"

#if defined IGRAPHICS_GL
  #if defined OS_MAC
    #if defined IGRAPHICS_GL2
      #define NANOVG_GL2_IMPLEMENTATION
    #elif defined IGRAPHICS_GL3
      #define NANOVG_GL3_IMPLEMENTATION
    #else
      #error Define either IGRAPHICS_GL2 or IGRAPHICS_GL3 for IGRAPHICS_NANOVG with OS_MAC
    #endif
  #elif defined OS_IOS
//    #if defined IGRAPHICS_GLES2
//      #include <OpenGLES/ES2/gl.h>
//      #define NANOVG_GLES2_IMPLEMENTATION
//    #elif defined IGRAPHICS_GLES3
//      #include <OpenGLES/ES3/gl.h>
//      #define NANOVG_GLES2_IMPLEMENTATION
//    #else
//      #error Define either IGRAPHICS_GLES2 or IGRAPHICS_GLES3 when using IGRAPHICS_GL and IGRAPHICS_NANOVG with OS_IOS
//    #endif
    #error NOT IMPLEMENTED
  #elif defined OS_WIN
    #pragma comment(lib, "opengl32.lib")
    #if defined IGRAPHICS_GL2
      #define NANOVG_GL2_IMPLEMENTATION
    #elif defined IGRAPHICS_GL3
      #define NANOVG_GL3_IMPLEMENTATION
    #else
      #error Define either IGRAPHICS_GL2 or IGRAPHICS_GL3 when using IGRAPHICS_GL and IGRAPHICS_NANOVG with OS_WIN
    #endif
  #elif defined OS_LINUX
    #error NOT IMPLEMENTED
  #elif defined OS_WEB
    #define GLFW_INCLUDE_GLEXT
    #if defined IGRAPHICS_GLES2
      #define GLFW_INCLUDE_ES2
      #define NANOVG_GLES2_IMPLEMENTATION
    #elif defined IGRAPHICS_GLES3
      #define NANOVG_GLES3_IMPLEMENTATION
      #define GLFW_INCLUDE_ES3
    #else
      #error Define either IGRAPHICS_GLES2 or IGRAPHICS_GLES3 when using IGRAPHICS_GL and IGRAPHICS_NANOVG with OS_WEB
    #endif
    #include <GLFW/glfw3.h>
    GLFWwindow* gWindow;
    void GLFWError(int error, const char* desc) { DBGMSG("GLFW error %d: %s\n", error, desc); }
  #endif
  #include "nanovg_gl.h"
  #include "nanovg_gl_utils.h"
#elif defined IGRAPHICS_METAL
  #if defined OS_MAC || defined OS_IOS
    #include "nanovg_mtl.h"
  #else
    #error NOT IMPLEMENTED
  #endif
#else
  #error you must define either IGRAPHICS_GL2, IGRAPHICS_GLES2 etc or IGRAPHICS_METAL when using IGRAPHICS_NANOVG
#endif

void nvgReadPixels(NVGcontext* pContext, int image, int x, int y, int width, int height, void* pData)
{
#if defined(IGRAPHICS_GL)
  glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pData);
#elif defined(IGRAPHICS_METAL)
  mnvgReadPixels(pContext, image, x, y, width, height, pData);
#endif
}

NanoVGBitmap::NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale, int nvgImageID)
{
  assert(nvgImageID > 0);

  mVG = pContext;
  int w = 0, h = 0;
  nvgImageSize(mVG, nvgImageID, &w, &h);
  
  SetBitmap(nvgImageID, w, h, sourceScale, 1.f);
}

NanoVGBitmap::NanoVGBitmap(NVGcontext* pContext, int width, int height, int scale, float drawScale)
{
  mVG = pContext;
  mFBO = nvgCreateFramebuffer(pContext, width, height, 0);
  
  nvgEndFrame(mVG);
  nvgBindFramebuffer(mFBO);
  
#ifdef IGRAPHICS_METAL
  mnvgClearWithColor(mVG, nvgRGBAf(0, 0, 0, 0));
#else
  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif
  nvgBeginFrame(mVG, width, height, 1.f);
  nvgEndFrame(mVG);
  
  SetBitmap(mFBO->image, width, height, scale, drawScale);
}

NanoVGBitmap::NanoVGBitmap(NVGcontext* pContext, int width, int height, const uint8_t* pData, int scale, float drawScale)
{
  int idx = nvgCreateImageRGBA(pContext, width, height, 0, pData);
  mVG = pContext;
  SetBitmap(idx, width, height, scale, drawScale);
}

NanoVGBitmap::~NanoVGBitmap()
{
  if(mFBO)
    nvgDeleteFramebuffer(mFBO);
  else
    nvgDeleteImage(mVG, GetBitmap());
}

#pragma mark -

// Utility conversions

inline NVGcolor NanoVGColor(const IColor& color, const IBlend* pBlend = 0)
{
  NVGcolor c;
  c.r = (float) color.R / 255.0f;
  c.g = (float) color.G / 255.0f;
  c.b = (float) color.B / 255.0f;
  c.a = (BlendWeight(pBlend) * color.A) / 255.0f;
  return c;
}

inline void NanoVGSetBlendMode(NVGcontext* context, const IBlend* pBlend)
{
  if (!pBlend)
  {
    nvgGlobalCompositeOperation(context, NVG_SOURCE_OVER);
      return;
  }
  
  switch (pBlend->mMethod)
  {
    case kBlendClobber:
      nvgGlobalCompositeBlendFunc(context, NVG_SRC_ALPHA, NVG_ONE_MINUS_SRC_ALPHA);
      break;
    case kBlendAdd:
      nvgGlobalCompositeBlendFunc(context, NVG_ONE, NVG_ONE);
      break;
    case kBlendUnder:
      nvgGlobalCompositeOperation(context, NVG_DESTINATION_OVER);
      break;
    case kBlendSourceIn:
      nvgGlobalCompositeOperation(context, NVG_SOURCE_IN);
      break;
    case kBlendColorDodge:
    case kBlendNone:
    default:
    {
      nvgGlobalCompositeOperation(context, NVG_SOURCE_OVER);
    }
  }
}

NVGpaint NanoVGPaint(NVGcontext* pContext, const IPattern& pattern, const IBlend* pBlend)
{
  double s[2], e[2];
  
  NVGcolor icol = NanoVGColor(pattern.GetStop(0).mColor, pBlend);
  NVGcolor ocol = NanoVGColor(pattern.GetStop(pattern.NStops() - 1).mColor, pBlend);
    
  // Invert transform
  IMatrix inverse = IMatrix(pattern.mTransform).Invert();
  inverse.TransformPoint(s[0], s[1], 0.0, 0.0);

  if (pattern.mType == kRadialPattern)
  {
    return nvgRadialGradient(pContext, s[0], s[1], 0.0, inverse.mXX, icol, ocol);
  }
  else
  {
    inverse.TransformPoint(e[0], e[1], 0.0, 1.0);
    
    return nvgLinearGradient(pContext, s[0], s[1], e[0], e[1], icol, ocol);
  }
}

#pragma mark -

IGraphicsNanoVG::IGraphicsNanoVG(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
{
  DBGMSG("IGraphics NanoVG @ %i FPS\n", fps);
}

IGraphicsNanoVG::~IGraphicsNanoVG() 
{
}

const char* IGraphicsNanoVG::GetDrawingAPIStr()
{
#if defined IGRAPHICS_METAL
  return "NanoVG | Metal";
#else
  #if defined OS_WEB
    return "NanoVG | WebGL";
  #else
    #if defined IGRAPHICS_GL2
      return "NanoVG | OpenGL2";
    #elif defined IGRAPHICS_GL3
      return "NanoVG | OpenGL3";
    #elif defined IGRAPHICS_GLES2
      return "NanoVG | OpenGLES2";
    #elif defined IGRAPHICS_GLES3
      return "NanoVG | OpenGLES3";
    #endif
  #endif
#endif
}

bool IGraphicsNanoVG::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) || (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr);
}

IBitmap IGraphicsNanoVG::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale)
{
  if (targetScale == 0)
    targetScale = GetScreenScale();

  // NanoVG does not use the global static cache, since bitmaps are textures linked to a context
  APIBitmap* pAPIBitmap = mBitmapCache.Find(name, targetScale);
  
  // If the bitmap is not already cached at the targetScale
  if (!pAPIBitmap)
  {
    const char* ext = name + strlen(name) - 1;
    while (ext >= name && *ext != '.') --ext;
    ++ext;

    WDL_String fullPathOrResourceID;
    int sourceScale = 0;
    EResourceLocation resourceFound = SearchImageResource(name, ext, fullPathOrResourceID, targetScale, sourceScale);

    bool bitmapTypeSupported = BitmapExtSupported(ext);
    
    if(resourceFound == EResourceLocation::kNotFound || !bitmapTypeSupported)
      return IBitmap(); // return invalid IBitmap

    pAPIBitmap = LoadAPIBitmap(fullPathOrResourceID.Get(), sourceScale, resourceFound, ext);
    
    mBitmapCache.Add(pAPIBitmap, name, sourceScale);

    assert(pAPIBitmap);
  }
  
  return IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name);
}

APIBitmap* IGraphicsNanoVG::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
  int idx = 0;

#ifdef OS_WIN
  if (location == EResourceLocation::kWinBinary)
  {
    const void* pResData = nullptr;

    int size = 0;
    pResData = LoadWinResource(fileNameOrResID, ext, size);

    if (pResData)
      idx = nvgCreateImageMem(mVG, 0 /*flags*/, (unsigned char*)pResData, size);
  }
  else
#endif
  if (location == EResourceLocation::kAbsolutePath)
  {
    idx = nvgCreateImage(mVG, fileNameOrResID, 0);
  }

  return new NanoVGBitmap(mVG, fileNameOrResID, scale, idx);
}

APIBitmap* IGraphicsNanoVG::CreateAPIBitmap(int width, int height)
{
  const double scale = GetBackingPixelScale();
  return new NanoVGBitmap(mVG, std::round(width * scale), std::round(height * scale), GetScreenScale(), GetDrawScale());
}

void IGraphicsNanoVG::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  int size = pBitmap->GetWidth() * pBitmap->GetHeight() * 4;
  
  data.Resize(size);
  
  if (data.GetSize() >= size)
  {
    PushLayer(layer.get(), false);
    nvgReadPixels(mVG, pBitmap->GetBitmap(), 0, 0, pBitmap->GetWidth(), pBitmap->GetHeight(), data.Get());
    PopLayer(false);    
  }
}

void IGraphicsNanoVG::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  int width = pBitmap->GetWidth();
  int height = pBitmap->GetHeight();
  int size = width * height * 4;
  
  if (mask.GetSize() >= size)
  {
    if (!shadow.mDrawForeground)
    {
      //pBitmap->GetBitmap()->clear(0);
    }
    
    IRECT bounds(layer->Bounds());
    
    NanoVGBitmap maskRawBitmap(mVG, width, height, mask.Get(), pBitmap->GetScale(), pBitmap->GetDrawScale());
    APIBitmap* shadowBitmap = new NanoVGBitmap(mVG, width, height, pBitmap->GetScale(), pBitmap->GetDrawScale());
    IBitmap tempLayerBitmap(shadowBitmap, 1, false);
    IBitmap maskBitmap(&maskRawBitmap, 1, false);
    ILayer shadowLayer(shadowBitmap, layer->Bounds());
    
    PathTransformSave();
    PushLayer(layer.get(), false);
    PushLayer(&shadowLayer, false);
    DrawBitmap(maskBitmap, bounds, 0, 0, nullptr);
    IBlend blend1(kBlendSourceIn, 1.0);
    PathRect(layer->Bounds());
    PathFill(shadow.mPattern, IFillOptions(), &blend1);
    PopLayer(false);
    IBlend blend2(kBlendUnder, shadow.mOpacity);
    bounds.Translate(shadow.mXOffset, shadow.mYOffset);
    DrawBitmap(tempLayerBitmap, bounds, 0, 0, &blend2);
    PopLayer(false);
    PathTransformRestore();
  }
}

void IGraphicsNanoVG::SetPlatformContext(void* pContext)
{
  mPlatformContext = pContext;
#ifdef OS_WIN
  if(pContext)
    OnViewInitialized(pContext);
#endif
}

void IGraphicsNanoVG::OnViewInitialized(void* pContext)
{
#if defined OS_WIN
  if (pContext)
  {
    PIXELFORMATDESCRIPTOR pfd =
    {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
      PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
      32, // Colordepth of the framebuffer.
      0, 0, 0, 0, 0, 0,
      0,
      0,
      0,
      0, 0, 0, 0,
      24, // Number of bits for the depthbuffer
      8, // Number of bits for the stencilbuffer
      0, // Number of Aux buffers in the framebuffer.
      PFD_MAIN_PLANE,
      0,
      0, 0, 0
    };
    
    HDC dc = (HDC) pContext;
    
    int fmt = ChoosePixelFormat(dc, &pfd);
    SetPixelFormat(dc, fmt, &pfd);
    
    mHGLRC = wglCreateContext(dc);
    wglMakeCurrent(dc, mHGLRC);
    if (!gladLoadGL())
      throw std::runtime_error{"Error initializing glad"};
    glGetError();
  }
#elif defined OS_WEB
  if (!glfwInit())
  {
    DBGMSG("Failed to init GLFW.");
    return;
  }

  glfwSetErrorCallback(GLFWError);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  gWindow = glfwCreateWindow(WindowWidth(), WindowHeight(), "NanoVG", NULL, NULL);

  if (!gWindow)
  {
    glfwTerminate();
    return;
  }

//  glfwSetKeyCallback(gWindow, key);
  glfwMakeContextCurrent(gWindow);
#endif // OS_WEB
 
  int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES /*| NVG_TRIPLE_BUFFER check!*/;
  
#if defined IGRAPHICS_METAL
  mVG = nvgCreateContext(pContext, flags);
#else
  mVG = nvgCreateContext(flags);
#endif
  
  if (mVG == nullptr)
    DBGMSG("Could not init nanovg.\n");
}

void IGraphicsNanoVG::OnViewDestroyed()
{
  // need to remove all the controls to free framebuffers, before deleting context
  RemoveAllControls();

  if(mMainFrameBuffer != nullptr)
    nvgDeleteFramebuffer(mMainFrameBuffer);
  
  mMainFrameBuffer = nullptr;
  
  if(mVG)
    nvgDeleteContext(mVG);
  
  mVG = nullptr;
  
#if defined OS_WIN
  if (mHGLRC)
  {
    wglMakeCurrent((HDC)mPlatformContext, nullptr);
    wglDeleteContext(mHGLRC);
  }
#elif defined OS_WEB
  glfwTerminate();
#endif
}

void IGraphicsNanoVG::DrawResize()
{
  if (mMainFrameBuffer != nullptr)
    nvgDeleteFramebuffer(mMainFrameBuffer);
  
  mMainFrameBuffer = nvgCreateFramebuffer(mVG, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale(), 0);
  
  if (mMainFrameBuffer == nullptr)
    DBGMSG("Could not init FBO.\n");
}

void IGraphicsNanoVG::BeginFrame()
{
  IGraphics::BeginFrame(); // start perf graph timing

#ifdef IGRAPHICS_METAL
  //  mnvgClearWithColor(mVG, nvgRGBAf(0, 0, 0, 0));
#else
  glViewport(0, 0, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  #ifdef OS_WEB
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  #endif
#endif
  
  nvgBindFramebuffer(mMainFrameBuffer); // begin main frame buffer update
  nvgBeginFrame(mVG, WindowWidth(), WindowHeight(), GetScreenScale());
}

void IGraphicsNanoVG::EndFrame()
{
  nvgEndFrame(mVG); // end main frame buffer update
  nvgBindFramebuffer(nullptr);
  
  nvgBeginFrame(mVG, WindowWidth(), WindowHeight(), GetScreenScale());

  NVGpaint img = nvgImagePattern(mVG, 0, 0, WindowWidth(), WindowHeight(), 0, mMainFrameBuffer->image, 1.0f);
  
  nvgSave(mVG);
  nvgResetTransform(mVG);
  nvgBeginPath(mVG);
  nvgRect(mVG, 0, 0, WindowWidth(), WindowHeight());
  nvgFillPaint(mVG, img);
  nvgFill(mVG);
  nvgRestore(mVG);
  
  nvgEndFrame(mVG);

#if defined OS_WEB
  glEnable(GL_DEPTH_TEST);
#endif
}

void IGraphicsNanoVG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  APIBitmap* pAPIBitmap = bitmap.GetAPIBitmap();
  
  assert(pAPIBitmap);
    
  // First generate a scaled image paint
    
  NVGpaint imgPaint;
  double scale = 1.0 / (pAPIBitmap->GetScale() * pAPIBitmap->GetDrawScale());

  nvgTransformScale(imgPaint.xform, scale, scale);

  scale *= GetScreenScale();
  imgPaint.xform[4] = dest.L - (srcX * scale);
  imgPaint.xform[5] = dest.T - (srcY * scale);
  imgPaint.extent[0] = bitmap.W() * bitmap.GetScale();
  imgPaint.extent[1] = bitmap.H() * bitmap.GetScale();
  imgPaint.image = pAPIBitmap->GetBitmap();
  imgPaint.radius = imgPaint.feather = 0.f;
  imgPaint.innerColor = imgPaint.outerColor = nvgRGBAf(1, 1, 1, BlendWeight(pBlend));
    
  // Now draw
    
  nvgBeginPath(mVG); // Clears any existing path
  nvgRect(mVG, dest.L, dest.T, dest.W(), dest.H());
  nvgFillPaint(mVG, imgPaint);
  NanoVGSetBlendMode(mVG, pBlend);
  nvgFill(mVG);
  nvgGlobalCompositeOperation(mVG, NVG_SOURCE_OVER);
  nvgBeginPath(mVG); // Clears the bitmap rect from the path state
}

void IGraphicsNanoVG::PathClear()
{
  nvgBeginPath(mVG);
}

void IGraphicsNanoVG::PathClose()
{
  nvgClosePath(mVG);
}

void IGraphicsNanoVG::PathArc(float cx, float cy, float r, float aMin, float aMax)
{
  nvgArc(mVG, cx, cy, r, DegToRad(aMin - 90.f), DegToRad(aMax - 90.f), NVG_CW);
}

void IGraphicsNanoVG::PathMoveTo(float x, float y)
{
  nvgMoveTo(mVG, x, y);
}

void IGraphicsNanoVG::PathLineTo(float x, float y)
{
  nvgLineTo(mVG, x, y);
}

void IGraphicsNanoVG::PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
  nvgBezierTo(mVG, x1, y1, x2, y2, x3, y3);
}

IColor IGraphicsNanoVG::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsNanoVG::DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
  assert(nvgFindFont(mVG, text.mFont) != -1); // did you forget to LoadFont for this font name?
  
  nvgFontBlur(mVG, 0);
  nvgFontSize(mVG, text.mSize);
  nvgFontFace(mVG, text.mFont);
  
  if(GetTextEntryControl() && GetTextEntryControl()->GetRECT() == bounds)
    nvgFillColor(mVG, NanoVGColor(text.mTextEntryFGColor, pBlend));
  else
    nvgFillColor(mVG, NanoVGColor(text.mFGColor, pBlend));
  
  float xpos = 0.;
  float ypos = 0.;
  
  int align = 0;
  switch (text.mAlign)
  {
    case IText::kAlignNear: align = NVG_ALIGN_LEFT; xpos = bounds.L; break;
    case IText::kAlignCenter: align = NVG_ALIGN_CENTER; xpos = bounds.MW(); break;
    case IText::kAlignFar: align = NVG_ALIGN_RIGHT; xpos = bounds.R; break;
    default:
      break;
  }
  
  switch (text.mVAlign)
  {
    case IText::kVAlignBottom: align |= NVG_ALIGN_BOTTOM; ypos = bounds.B; break;
    case IText::kVAlignMiddle: align |= NVG_ALIGN_MIDDLE; ypos = bounds.MH(); break;
    case IText::kVAlignTop: align |= NVG_ALIGN_TOP; ypos = bounds.T; break;
    default: break;
      break;
  }
  
  nvgTextAlign(mVG, align);
  
  auto calcTextBounds = [&](IRECT& r)
  {
    float fbounds[4];
    nvgTextBounds(mVG, xpos, ypos, str, NULL, fbounds);
    r.L = fbounds[0]; r.T = fbounds[1]; r.R = fbounds[2]; r.B = fbounds[3];
  };
  
  if(measure)
  {
    calcTextBounds(bounds);
    return true;
  }
  else
  {
    NanoVGSetBlendMode(mVG, pBlend);
      
    if(text.mOrientation != 0)
    {
      IRECT tmp;
      calcTextBounds(tmp);

      nvgSave(mVG);
      nvgTranslate(mVG, tmp.L, tmp.B);
      nvgRotate(mVG, nvgDegToRad(text.mOrientation));
      nvgTranslate(mVG, -tmp.L, -tmp.B);
      nvgText(mVG, xpos, ypos, str, NULL);
      nvgRestore(mVG);
    }
    else
      nvgText(mVG, xpos, ypos, str, NULL);
      
    nvgGlobalCompositeOperation(mVG, NVG_SOURCE_OVER);
  }
    
  return true;
}

void IGraphicsNanoVG::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  // First set options
  switch (options.mCapOption)
  {
    case kCapButt:   nvgLineCap(mVG, NVG_BUTT);     break;
    case kCapRound:  nvgLineCap(mVG, NVG_ROUND);    break;
    case kCapSquare: nvgLineCap(mVG, NVG_SQUARE);   break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter: nvgLineJoin(mVG, NVG_MITER);   break;
    case kJoinRound: nvgLineJoin(mVG, NVG_ROUND);   break;
    case kJoinBevel: nvgLineJoin(mVG, NVG_BEVEL);   break;
  }
  
  nvgMiterLimit(mVG, options.mMiterLimit);
  nvgStrokeWidth(mVG, thickness);
 
  // NanoVG does not support dashed paths
  if (pattern.mType == kSolidPattern)
    nvgStrokeColor(mVG, NanoVGColor(pattern.GetStop(0).mColor, pBlend));
  else
    nvgStrokePaint(mVG, NanoVGPaint(mVG, pattern, pBlend));
  
  nvgPathWinding(mVG, NVG_CCW);
  NanoVGSetBlendMode(mVG, pBlend);
  nvgStroke(mVG);
  nvgGlobalCompositeOperation(mVG, NVG_SOURCE_OVER);
    
  if (!options.mPreserve)
    nvgBeginPath(mVG); // Clears the path state
}

void IGraphicsNanoVG::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  nvgPathWinding(mVG, options.mFillRule == kFillWinding ? NVG_CCW : NVG_CW);
  
  if (pattern.mType == kSolidPattern)
    nvgFillColor(mVG, NanoVGColor(pattern.GetStop(0).mColor, pBlend));
  else
    nvgFillPaint(mVG, NanoVGPaint(mVG, pattern, pBlend));
  
  NanoVGSetBlendMode(mVG, pBlend);
  nvgFill(mVG);
  nvgGlobalCompositeOperation(mVG, NVG_SOURCE_OVER);

  if (!options.mPreserve)
    nvgBeginPath(mVG); // Clears the path state
}

bool IGraphicsNanoVG::LoadFont(const char* fileName)
{
  // does not check for existing fonts
  WDL_String fontNameWithoutExt(fileName, (int) strlen(fileName));
  fontNameWithoutExt.remove_fileext();
  WDL_String fullPath;
  EResourceLocation foundResource = OSFindResource(fileName, "ttf", fullPath);
 
  if (foundResource != EResourceLocation::kNotFound)
  {
    int fontID = -1;

#ifdef OS_WIN
    if(foundResource == EResourceLocation::kWinBinary)
    {
      int sizeInBytes = 0;
      const void* pResData = LoadWinResource(fullPath.Get(), "ttf", sizeInBytes);

      if(pResData && sizeInBytes)
        fontID = nvgCreateFontMem(mVG, fontNameWithoutExt.Get(), (unsigned char*) pResData, sizeInBytes, 0 /* ?? */);

      if(fontID == -1)
        return false;
    }
    else
#endif
    fontID = nvgCreateFont(mVG, fontNameWithoutExt.Get(), fullPath.Get());

    if (fontID == -1)
    {
      DBGMSG("Could not locate font %s\n", fileName);
      return false;
    }
    else
      return true;
  }

  return false;
}

void IGraphicsNanoVG::DrawBoxShadow(const IRECT& bounds, float cr, float ydrop, float pad, const IBlend* pBlend)
{
  IRECT inner = bounds.GetPadded(-pad);
  NVGpaint shadowPaint = nvgBoxGradient(mVG, inner.L, inner.T + ydrop, inner.W(), inner.H(), cr * 2., 20, NanoVGColor(COLOR_BLACK_DROP_SHADOW, pBlend), NanoVGColor(COLOR_TRANSPARENT));
  nvgBeginPath(mVG);
  nvgRect(mVG, bounds.L, bounds.T, bounds.W(), bounds.H());
  nvgRoundedRect(mVG, inner.L, inner.T, inner.W(), inner.H(), cr);
  nvgPathWinding(mVG, NVG_HOLE);
  nvgFillPaint(mVG, shadowPaint);
  NanoVGSetBlendMode(mVG, pBlend);
  nvgFill(mVG);
  nvgGlobalCompositeOperation(mVG, NVG_SOURCE_OVER);
  nvgBeginPath(mVG); // Clear the paths
}

void IGraphicsNanoVG::UpdateLayer()
{
  if (mLayers.empty())
  {
    nvgEndFrame(mVG);
#ifndef IGRAPHICS_METAL
    glViewport(0, 0, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
#endif
    nvgBindFramebuffer(mMainFrameBuffer);
    nvgBeginFrame(mVG, WindowWidth(), WindowHeight(), GetScreenScale());
  }
  else
  {
    nvgEndFrame(mVG);
#ifndef IGRAPHICS_METAL
    const double scale = GetBackingPixelScale();
    glViewport(0, 0, mLayers.top()->Bounds().W() * scale, mLayers.top()->Bounds().H() * scale);
#endif
    nvgBindFramebuffer(dynamic_cast<const NanoVGBitmap*>(mLayers.top()->GetAPIBitmap())->GetFBO());
    nvgBeginFrame(mVG, mLayers.top()->Bounds().W() * GetDrawScale(), mLayers.top()->Bounds().H() * GetDrawScale(), GetScreenScale());
  }
}

void IGraphicsNanoVG::PathTransformSetMatrix(const IMatrix& m)
{
  double xTranslate = 0.0;
  double yTranslate = 0.0;
  
  if (!mLayers.empty())
  {
    IRECT bounds = mLayers.top()->Bounds();
    
    xTranslate = -bounds.L;
    yTranslate = -bounds.T;
  }
  
  nvgResetTransform(mVG);
  nvgScale(mVG, GetDrawScale(), GetDrawScale());
  nvgTranslate(mVG, xTranslate, yTranslate);
  nvgTransform(mVG, m.mXX, m.mYX, m.mXY, m.mYY, m.mTX, m.mTY);
}

void IGraphicsNanoVG::SetClipRegion(const IRECT& r)
{
  if (!r.Empty())
    nvgScissor(mVG, r.L, r.T, r.W(), r.H());
  else
    nvgResetScissor(mVG);
}

void IGraphicsNanoVG::DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen)
{
  const float xd = x1 - x2;
  const float yd = y1 - y2;
  const float len = std::sqrt(xd * xd + yd * yd);
  
  const float segs = std::round(len / dashLen);
  const float incr = 1.f / segs;

  float xs = x1;
  float ys = y1;

  PathMoveTo(xs, ys);

  for (int i = 1; i < static_cast<int>(segs); i+=2)
  {
    float progress = incr * static_cast<float>(i);
  
    float xe = x1 + progress * (x2 - x1);
    float ye = y1 + progress * (y2 - y1);
    
    PathLineTo(xe, ye);
    
    progress += incr;
    
    xs = x1 + progress * (x2 - x1);;
    ys = y1 + progress * (y2 - y1);
    
    PathMoveTo(xs, ys);
  }
  
  PathStroke(color, thickness, IStrokeOptions(), pBlend);
}

void IGraphicsNanoVG::DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen)
{
  const int xsegs = bounds.W() / (dashLen * 2.f);
  const int ysegs = bounds.H() / (dashLen * 2.f);

  float x1 = bounds.L;
  float y1 = bounds.T;
  
  float x2 = x1;
  float y2 = y1;
  
  PathMoveTo(x1, y1);

  for(int j = 0; j < 2; j++)
  {
    for (int i = 0; i < xsegs; i++)
    {
      x2 = x1 + dashLen;
      PathLineTo(x2, y2);
      x1 = x2 + dashLen;
      PathMoveTo(x1, y1);
    }
    
    x2 = x1;
    
    for (int i = 0; i < ysegs; i++)
    {
      y2 = y1 + dashLen;
      PathLineTo(x2, y2);
      y1 = y2 + dashLen;
      PathMoveTo(x1, y1);
    }
    
    y2 = y1;
    
    dashLen = -dashLen;
  }
  
  PathStroke(color, thickness, IStrokeOptions(), pBlend);
}
