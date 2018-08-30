#include <cmath>

#include "IGraphicsNanoVG.h"

#if defined OS_MAC || defined OS_IOS
  #include "nanovg_mtl.h"
#elif defined OS_WIN
  #pragma comment(lib, "opengl32.lib")
  #define NANOVG_GL2_IMPLEMENTATION
  #include <glad/glad.h>
  #include "nanovg_gl.h"
#elif defined OS_WEB
  #define GLFW_INCLUDE_ES2
  #define GLFW_INCLUDE_GLEXT
  #include <GLFW/glfw3.h>
  #define NANOVG_GLES2_IMPLEMENTATION
  #include "nanovg_gl.h"
  #include "nanovg_gl_utils.h"
  GLFWwindow* gWindow;
  void GLFWError(int error, const char* desc) { DBGMSG("GLFW error %d: %s\n", error, desc); }
#else
  #error platform not yet supported
#endif

#pragma mark -

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

NVGpaint NanoVGPaint(NVGcontext* context, const IPattern& pattern, const IBlend* pBlend)
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
    return nvgRadialGradient(context, s[0], s[1], 0.0, inverse[0], icol, ocol);
  }
  else
  {
    float e[2];
    nvgTransformPoint(&e[0], &e[1], inverse, 1, 0);
    
    return nvgLinearGradient(context, s[0], s[1], e[0], e[1], icol, ocol);
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
    const int targetScale = round(GetDisplayScale());
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

void IGraphicsNanoVG::SetPlatformContext(void* pContext) {
  mPlatformContext = pContext;
#ifdef OS_WIN
  if(pContext)
    OnViewInitialized(pContext);
#endif
}

void IGraphicsNanoVG::OnViewInitialized(void* pContext)
{
#if defined OS_MAC || defined OS_IOS
  mVG = nvgCreateMTL(pContext, NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_TRIPLE_BUFFER /*check!*/);
#elif defined OS_WIN
  if (pContext) {
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
    mVG = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  }
#elif defined OS_WEB
  if (!glfwInit()) {
    DBGMSG("Failed to init GLFW.");
    return;
  }

  glfwSetErrorCallback(GLFWError);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  gWindow = glfwCreateWindow(Width(), Height(), "NanoVG", NULL, NULL);

  if (!gWindow) {
    glfwTerminate();
    return;
  }

//  glfwSetKeyCallback(gWindow, key);
  glfwMakeContextCurrent(gWindow);
  
  mVG = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (mVG == nullptr) {
    DBGMSG("Could not init nanovg.\n");
  }
  
#endif
}

void IGraphicsNanoVG::OnViewDestroyed()
{
#if defined OS_MAC || defined OS_IOS
  if(mVG)
    nvgDeleteMTL(mVG);
#elif defined OS_WIN
  if (mVG)
    nvgDeleteGL2(mVG);
  if (mHGLRC) {
    wglMakeCurrent((HDC)mPlatformContext, nullptr);
    wglDeleteContext(mHGLRC);
  }
#elif defined OS_WEB
  nvgDeleteGLES2(mVG);
  glfwTerminate();
#endif
}

void IGraphicsNanoVG::BeginFrame()
{
  IGraphics::BeginFrame(); // perf graph

#ifdef OS_WIN
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glViewport(0, 0, Width()*GetDisplayScale(), Height()*GetDisplayScale());
#elif defined OS_WEB
  glViewport(0, 0, Width() * GetDisplayScale(), Height() * GetDisplayScale());
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
#endif
  
  nvgBeginFrame(mVG, WindowWidth(), WindowHeight(), GetDisplayScale());

  const float scale = GetScale();
  
  nvgScale(mVG, scale, scale);
}

void IGraphicsNanoVG::EndFrame()
{
  nvgEndFrame(mVG);

#if defined OS_WEB
  glEnable(GL_DEPTH_TEST);
#endif
}

void IGraphicsNanoVG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  APIBitmap* pAPIBitmap = bitmap.GetAPIBitmap();
  
  NVGpaint imgPaint = nvgImagePattern(mVG, std::round(dest.L) - srcX, std::round(dest.T) - srcY, bitmap.W(), bitmap.H(), 0.f, pAPIBitmap->GetBitmap(), BlendWeight(pBlend));
  nvgBeginPath(mVG); // Clear's any existing path
  nvgRect(mVG, dest.L, dest.T, dest.W(), dest.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
  nvgBeginPath(mVG); // Clear's the bitmap rect from the path state
}

IColor IGraphicsNanoVG::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsNanoVG::DrawText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
  assert(nvgFindFont(mVG, text.mFont) != -1); // did you forget to LoadFont for this font name?
  
  nvgFontBlur(mVG, 0);
  nvgFontSize(mVG, text.mSize);
  nvgFontFace(mVG, text.mFont);
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
  
  if(measure)
  {
    float fbounds[4];
    nvgTextBounds(mVG, xpos, ypos, str, NULL, fbounds);
    bounds.L = fbounds[0]; bounds.T = fbounds[1]; bounds.R = fbounds[2]; bounds.B = fbounds[3];
    return true;
  }
  else
    nvgText(mVG, xpos, ypos, str, NULL);

  return true;
}

bool IGraphicsNanoVG::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, 0, true);
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
    PathClear();
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
    PathClear();
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
  nvgBeginPath(mVG); // GAH! actually consuming
}
