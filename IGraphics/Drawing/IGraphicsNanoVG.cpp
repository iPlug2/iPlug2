#include <cmath>

#include "IGraphicsNanoVG.h"

#if defined IGRAPHICS_GL
  #if defined OS_MAC
//    #if defined IGRAPHICS_GL2
//      #define NANOVG_GL2_IMPLEMENTATION
//    #elif defined IGRAPHICS_GL3
//      #define NANOVG_GL3_IMPLEMENTATION
//    #else
//      #error Define either IGRAPHICS_GL2 or IGRAPHICS_GL3 when using IGRAPHICS_GL and IGRAPHICS_NANOVG with OS_MAC
//    #endif
    #error NOT IMPLEMENTED
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
  #error you must define either IGRAPHICS_GL or IGRAPHICS_METAL when using IGRAPHICS_NANOVG
#endif

#ifdef OS_WIN
int LoadImageFromWinResource(NVGcontext* pContext, HINSTANCE hInst, const char* resid)
{
  HRSRC hResource = FindResource(hInst, resid, "PNG");
  if (!hResource) return NULL;

  DWORD imageSize = SizeofResource(hInst, hResource);
  if (imageSize < 8) return NULL;

  HGLOBAL res = LoadResource(hInst, hResource);
  const void* pResourceData = LockResource(res);
  if (!pResourceData) return NULL;

  int ret = nvgCreateImageMem(pContext, 0 /*flags*/, (unsigned char*) pResourceData, imageSize);

  return ret;
}

int LoadFontFromWinResource(NVGcontext* pContext, HINSTANCE hInst, const char* name, const char* resid)
{
  HRSRC hResource = FindResource(hInst, resid, "TTF");
  if (!hResource) return NULL;

  DWORD fontSize = SizeofResource(hInst, hResource);
  if (fontSize < 8) return NULL;

  HGLOBAL res = LoadResource(hInst, hResource);
  const void* pResourceData = LockResource(res);
  if (!pResourceData) return NULL;

  int ret = nvgCreateFontMem(pContext, name, (unsigned char*)pResourceData, fontSize, 0 /* ?? */);
  return ret;
}
#endif

NanoVGBitmap::NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale, void* hInst)
{
  mVG = pContext;
  int w = 0, h = 0;
#ifdef OS_WIN
  int idx = LoadImageFromWinResource(pContext, (HINSTANCE)hInst, path); // TODO: then try absolute path?
#else
  int idx = nvgCreateImage(mVG, path, 0);
#endif
  nvgImageSize(mVG, idx, &w, &h);
  
  SetBitmap(idx, w, h, sourceScale);
}

NanoVGBitmap::~NanoVGBitmap()
{
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

inline NVGcompositeOperation NanoVGBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    return NVG_COPY;
  }
  
  switch (pBlend->mMethod)
  {
    case kBlendClobber:
    {
      return NVG_SOURCE_OVER;
    }
    case kBlendAdd:
    case kBlendColorDodge:
    case kBlendNone:
    default:
    {
      return NVG_COPY;
    }
  }
}

NVGpaint NanoVGPaint(NVGcontext* pContext, const IPattern& pattern, const IBlend* pBlend)
{
  NVGcolor icol = NanoVGColor(pattern.GetStop(0).mColor, pBlend);
  NVGcolor ocol = NanoVGColor(pattern.GetStop(pattern.NStops() - 1).mColor, pBlend);
  
  // Invert transform
  
  float inverse[6];
  nvgTransformInverse(inverse, pattern.mTransform);
  float s[2];
  
  nvgTransformPoint(&s[0], &s[1], inverse, 0, 0);
  
  if (pattern.mType == kRadialPattern)
  {
    return nvgRadialGradient(pContext, s[0], s[1], 0.0, inverse[0], icol, ocol);
  }
  else
  {
    float e[2];
    nvgTransformPoint(&e[0], &e[1], inverse, 1, 0);
    
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

IBitmap IGraphicsNanoVG::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal)
{
  const int targetScale = round(GetDisplayScale());
  
  APIBitmap* pAPIBitmap = mBitmapCache.Find(name, targetScale);
  
  // If the bitmap is not already cached at the targetScale
  if (!pAPIBitmap)
  {
    WDL_String fullPath;
    int sourceScale = 0;
    bool resourceFound = SearchImageResource(name, "png", fullPath, targetScale, sourceScale);
    assert(resourceFound);
    
    pAPIBitmap = LoadAPIBitmap(fullPath, sourceScale);
    
    mBitmapCache.Add(pAPIBitmap, name, sourceScale);

    assert(pAPIBitmap);
  }
  
  return IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name);
}

APIBitmap* IGraphicsNanoVG::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new NanoVGBitmap(mVG, resourcePath.Get(), scale, GetPlatformInstance());
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
  if(mMainFrameBuffer != nullptr)
    nvgDeleteFramebuffer(mMainFrameBuffer);
  
  if(mVG)
    nvgDeleteContext(mVG);
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
  
  mMainFrameBuffer = nvgCreateFramebuffer(mVG, WindowWidth() * GetDisplayScale(), WindowHeight() * GetDisplayScale(), 0);
  
  if (mMainFrameBuffer == nullptr)
    DBGMSG("Could not init FBO.\n");
}

void IGraphicsNanoVG::BeginFrame()
{
  IGraphics::BeginFrame(); // perf graph

#ifdef OS_WIN
  glViewport(0, 0, WindowWidth() * GetDisplayScale(), WindowHeight() * GetDisplayScale());
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#elif defined OS_WEB
  glViewport(0, 0, WindowWidth() * GetDisplayScale(), WindowHeight() * GetDisplayScale());
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
#endif
  
  nvgBindFramebuffer(mMainFrameBuffer); // begin main frame buffer update
  nvgBeginFrame(mVG, WindowWidth(), WindowHeight(), GetDisplayScale());
}

void IGraphicsNanoVG::EndFrame()
{
  nvgEndFrame(mVG); // end main frame buffer update
  nvgBindFramebuffer(nullptr);
  
  nvgBeginFrame(mVG, WindowWidth(), WindowHeight(), GetDisplayScale());

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
  
  NVGpaint imgPaint = nvgImagePattern(mVG, std::round(dest.L) - srcX, std::round(dest.T) - srcY, bitmap.W(), bitmap.H(), 0.f, pAPIBitmap->GetBitmap(), BlendWeight(pBlend));
  nvgBeginPath(mVG); // Clears any existing path
  nvgRect(mVG, dest.L, dest.T, dest.W(), dest.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
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

bool IGraphicsNanoVG::DrawText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure, bool textEntry)
{
  assert(nvgFindFont(mVG, text.mFont) != -1); // did you forget to LoadFont for this font name?
  
  nvgFontBlur(mVG, 0);
  nvgFontSize(mVG, text.mSize);
  nvgFontFace(mVG, text.mFont);
  
  if(textEntry)
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
  }
    
  return true;
}

bool IGraphicsNanoVG::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, 0, true, false);
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
 
  // TODO Dash

  if (pattern.mType == kSolidPattern)
    nvgStrokeColor(mVG, NanoVGColor(pattern.GetStop(0).mColor, pBlend));
  else
    nvgStrokePaint(mVG, NanoVGPaint(mVG, pattern, pBlend));
  
  nvgPathWinding(mVG, NVG_CCW);
  nvgStroke(mVG);
  
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
  
  nvgFill(mVG);
  
  if (!options.mPreserve)
    nvgBeginPath(mVG); // Clears the path state
}

void IGraphicsNanoVG::LoadFont(const char* name)
{
  WDL_String fontNameWithoutExt(name, (int) strlen(name));
  fontNameWithoutExt.remove_fileext();
  WDL_String fullPath;
  OSFindResource(name, "ttf", fullPath);
  
  int fontID = -1;
  
  if (fullPath.GetLength())
  {
#ifdef OS_WIN
    fontID = LoadFontFromWinResource(mVG, (HINSTANCE) GetPlatformInstance(), fontNameWithoutExt.Get(), fullPath.Get());
#else
    fontID = nvgCreateFont(mVG, fontNameWithoutExt.Get(), fullPath.Get());
#endif
  }
  else {
    DBGMSG("Could not locate font %s\n", name);
  }
  
  assert (fontID != -1); // font not found!
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
  nvgFill(mVG);
  nvgBeginPath(mVG); // Clear the paths
}

void IGraphicsNanoVG::PathTransformSetMatrix(const IMatrix& m)
{
  nvgResetTransform(mVG);
  nvgScale(mVG, GetScale(), GetScale());
  nvgTransform(mVG, m.mTransform[0], m.mTransform[1], m.mTransform[2], m.mTransform[3], m.mTransform[4], m.mTransform[5]);
}

void IGraphicsNanoVG::SetClipRegion(const IRECT& r)
{
  if (!r.Empty())
    nvgScissor(mVG, r.L, r.T, r.W(), r.H());
  else
    nvgResetScissor(mVG);
}
