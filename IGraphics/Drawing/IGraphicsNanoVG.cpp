#include <cmath>

#include "IGraphicsNanoVG.h"
#ifdef OS_WIN
#pragma comment(lib, "opengl32.lib")
#define NANOVG_GL2_IMPLEMENTATION
#include <glad/glad.h>
#include "nanovg_gl.h"
#endif

#if NANOVG_PERF
#include "perf.c"
#endif

#pragma mark -

inline int GetBitmapIdx(APIBitmap* pBitmap) { return (int) ((long long) pBitmap->GetBitmap()); }

NanoVGBitmap::NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale)
{
  mVG = pContext;
  int w = 0, h = 0;
  int idx = nvgCreateImage(mVG, path, 0);
  nvgImageSize(mVG, idx, &w, &h);
      
  SetBitmap((void*) idx, w, h, sourceScale);
}

NanoVGBitmap::~NanoVGBitmap()
{
  int idx = GetBitmapIdx(this);
  nvgDeleteImage(mVG, idx);
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

IGraphicsNanoVG::IGraphicsNanoVG(IEditorDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
#if NANOVG_PERF
  mPerfGraph = new PerfGraph;
  initGraph(mPerfGraph, GRAPH_RENDER_FPS, "Frame Time");
 #endif
  
  DBGMSG("IGraphics NanoVG @ %i FPS\n", fps);
}

IGraphicsNanoVG::~IGraphicsNanoVG() 
{
  mBitmaps.Empty(true);
  
#if defined OS_MAC || defined OS_IOS
  if(mVG)
    nvgDeleteMTL(mVG);
#endif

#ifdef OS_WIN
  if (mVG)
    nvgDeleteGL2(mVG);
  if (mHGLRC) {
    wglMakeCurrent((HDC)mPlatformContext, nullptr);
    wglDeleteContext(mHGLRC);
  }
#endif

#if NANOVG_PERF
  delete mPerfGraph;
#endif
}

IBitmap IGraphicsNanoVG::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal)
{
  WDL_String fullPath;
  const int targetScale = round(GetDisplayScale());
  int sourceScale = 0;
  bool resourceFound = SearchImageResource(name, "png", fullPath, targetScale, sourceScale);
  assert(resourceFound);
    
  NanoVGBitmap* bitmap = (NanoVGBitmap*) LoadAPIBitmap(fullPath, sourceScale);
  assert(bitmap);
  mBitmaps.Add(bitmap);
  
  return IBitmap(bitmap, nStates, framesAreHorizontal, name);
}

APIBitmap* IGraphicsNanoVG::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new NanoVGBitmap(mVG, resourcePath.Get(), scale);
}

APIBitmap* IGraphicsNanoVG::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  return nullptr;
}

void IGraphicsNanoVG::RetainBitmap(const IBitmap& bitmap, const char* cacheName)
{
}

void IGraphicsNanoVG::SetPlatformContext(void* pContext) {
  mPlatformContext = pContext;
#ifdef OS_WIN
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

    HDC dc = (HDC)pContext;

    int fmt = ChoosePixelFormat(dc, &pfd);
    SetPixelFormat(dc, fmt, &pfd);

    mHGLRC = wglCreateContext(dc);
    wglMakeCurrent(dc, mHGLRC);
    if (!gladLoadGL())
      throw std::runtime_error{"Error initializing glad"};
    glGetError();
    mVG = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  }
#endif
}

IBitmap IGraphicsNanoVG::ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale)
{
  return bitmap;
}

void IGraphicsNanoVG::ViewInitialized(void* layer)
{
#if defined OS_MAC || defined OS_IOS
  mVG = nvgCreateMTL(layer, NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_TRIPLE_BUFFER /*check!*/);
#endif
}

static double GetTimestamp() {
  static auto start = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

void IGraphicsNanoVG::BeginFrame()
{
#if NANOVG_PERF
  mnvgClearWithColor(mVG, NanoVGColor(COLOR_BLACK));
  const double timestamp = GetTimestamp();
  const double timeDiff = timestamp - mPrevTimestamp;
  updateGraph(mPerfGraph, timeDiff);
  mPrevTimestamp = timestamp;
#endif
  
#ifdef OS_WIN
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glViewport(0, 0, Width()*GetDisplayScale(), Height()*GetDisplayScale());
#endif
  
  nvgBeginFrame(mVG, Width(), Height(), GetDisplayScale());
}

void IGraphicsNanoVG::EndFrame()
{
#if NANOVG_PERF
  renderGraph(mVG, 5, 5, mPerfGraph);
#endif
  nvgEndFrame(mVG);
}

void IGraphicsNanoVG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  int idx = GetBitmapIdx(bitmap.GetAPIBitmap());
  NVGpaint imgPaint = nvgImagePattern(mVG, std::round(dest.L) - srcX, std::round(dest.T) - srcY, bitmap.W(), bitmap.H(), 0.f, idx, BlendWeight(pBlend));
  PathClear();
  nvgRect(mVG, dest.L, dest.T, dest.W(), dest.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
  PathClear();
}

IColor IGraphicsNanoVG::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsNanoVG::DrawText(const IText& text, const char* str, IRECT& bounds, bool measure)
{
  assert(nvgFindFont(mVG, text.mFont) != -1); // did you forget to LoadFont for this font name?
  
  nvgFontBlur(mVG, 0);
  nvgFontSize(mVG, text.mSize);
  nvgFontFace(mVG, text.mFont);
  nvgFillColor(mVG, NanoVGColor(text.mFGColor));
  
  int align = 0;
  switch (text.mAlign) // todo valign
  {
    case IText::kAlignNear: align = NVG_ALIGN_LEFT; break;
    case IText::kAlignCenter: align = NVG_ALIGN_CENTER; break;
    case IText::kAlignFar: align = NVG_ALIGN_RIGHT; break;
    default:
      break;
  }
  
  switch (text.mVAlign) // todo valign
  {
    case IText::kVAlignBottom: align |= NVG_ALIGN_BOTTOM; break;
    case IText::kVAlignMiddle: align |= NVG_ALIGN_MIDDLE; break;
    case IText::kVAlignTop: align |= NVG_ALIGN_TOP; break;
    default:
      break;
  }
  
  nvgTextAlign(mVG, align | NVG_ALIGN_BASELINE);
  
  if(measure)
  {
    float fbounds[4];
    nvgTextBounds(mVG, 0., 0., str, NULL, fbounds);
    bounds.L = fbounds[0]; bounds.T = fbounds[1]; bounds.R = fbounds[2]; bounds.B = fbounds[3];
    return true;
  }
  else
    nvgText(mVG, bounds.MW() , bounds.MH(), str, NULL);

  return true;
}

bool IGraphicsNanoVG::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, true);
}

void IGraphicsNanoVG::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  // First set options
  switch (options.mCapOption)
  {
    case kCapButt:   nvgLineCap(mVG, NSVG_CAP_BUTT);     break;
    case kCapRound:  nvgLineCap(mVG, NSVG_CAP_ROUND);    break;
    case kCapSquare: nvgLineCap(mVG, NSVG_CAP_SQUARE);   break;
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
  
  if(fullPath.GetLength())
    fontID = nvgCreateFont(mVG, fontNameWithoutExt.Get(), fullPath.Get());
  else {
    DBGMSG("Could not locate font %s\n", name);
  }
  
  assert (fontID != -1); // font not found!
}

void IGraphicsNanoVG::DrawBoxShadow(const IRECT& bounds, float cr, float ydrop, float pad)
{
  IRECT inner = bounds.GetPadded(-pad);
  NVGpaint shadowPaint = nvgBoxGradient(mVG, inner.L, inner.T + ydrop, inner.W(), inner.H(), cr * 2., 20, NanoVGColor(COLOR_BLACK_DROP_SHADOW), NanoVGColor(COLOR_TRANSPARENT));
  nvgBeginPath(mVG);
  nvgRect(mVG, bounds.L, bounds.T, bounds.W(), bounds.H());
  nvgRoundedRect(mVG, inner.L, inner.T, inner.W(), inner.H(), cr);
  nvgPathWinding(mVG, NVG_HOLE);
  nvgFillPaint(mVG, shadowPaint);
  nvgFill(mVG);
  nvgBeginPath(mVG);
}
