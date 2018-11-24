#include <cmath>

#include "IGraphicsSkia.h"

SkiaBitmap::SkiaBitmap(const char* path, double sourceScale)
{

//  SetBitmap(idx, w, h, sourceScale);
}

SkiaBitmap::~SkiaBitmap()
{
}

#pragma mark -

// Utility conversions

inline SkColor SkiaColor(const IColor& color, const IBlend* pBlend = 0)
{
  return SkColorSetARGB(color.A, color.R, color.G, color.B);
}

//inline SkiaBlendMode(const IBlend* pBlend)
//{
//}

//NVGpaint SkiaPaint(NVGcontext* context, const IPattern& pattern, const IBlend* pBlend)
//{
//  NVGcolor icol = SkiaColor(pattern.GetStop(0).mColor, pBlend);
//  NVGcolor ocol = SkiaColor(pattern.GetStop(pattern.NStops() - 1).mColor, pBlend);
//
//  // Invert transform
//
//  float inverse[6];
//  nvgTransformInverse(inverse, pattern.mTransform);
//  float s[2];
//
//  nvgTransformPoint(&s[0], &s[1], inverse, 0, 0);
//
//  if (pattern.mType == kRadialPattern)
//  {
//    return nvgRadialGradient(context, s[0], s[1], 0.0, inverse[0], icol, ocol);
//  }
//  else
//  {
//    float e[2];
//    nvgTransformPoint(&e[0], &e[1], inverse, 1, 0);
//
//    return nvgLinearGradient(context, s[0], s[1], e[0], e[1], icol, ocol);
//  }
//}

#pragma mark -

IGraphicsSkia::IGraphicsSkia(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
{
  DBGMSG("IGraphics Skia @ %i FPS\n", fps);
}

IGraphicsSkia::~IGraphicsSkia()
{
}
//
//IBitmap IGraphicsSkia::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal)
//{
//  const int targetScale = round(GetDisplayScale());
//  
//  APIBitmap* pAPIBitmap = mBitmapCache.Find(name, targetScale);
//  
//  // If the bitmap is not already cached at the targetScale
//  if (!pAPIBitmap)
//  {
//    WDL_String fullPath;
//    int sourceScale = 0;
//    bool resourceFound = SearchImageResource(name, "png", fullPath, targetScale, sourceScale);
//    assert(resourceFound);
//    
//    pAPIBitmap = LoadAPIBitmap(fullPath, sourceScale);
//    
//    mBitmapCache.Add(pAPIBitmap, name, sourceScale);
//
//    assert(pAPIBitmap);
//  }
//  
//  return IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name);
//}

APIBitmap* IGraphicsSkia::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new SkiaBitmap(resourcePath.Get(), scale);
}

void IGraphicsSkia::SetPlatformContext(void* pContext)
{
  mPlatformContext = pContext;
}

void IGraphicsSkia::OnViewInitialized(void* pContext)
{
}

void IGraphicsSkia::OnViewDestroyed()
{
}

void IGraphicsSkia::DrawResize()
{
//  if(mSurface != nullptr);
  
  mSurface = SkSurface::MakeRasterN32Premul(WindowWidth() * GetDisplayScale(), WindowHeight() * GetDisplayScale());
}

void IGraphicsSkia::BeginFrame()
{
}

void IGraphicsSkia::EndFrame()
{
}

void IGraphicsSkia::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
}

IColor IGraphicsSkia::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsSkia::DrawText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
}

bool IGraphicsSkia::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, 0, true);
}

void IGraphicsSkia::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  SkPaint p;
  p.setAntiAlias(true);
  mSurface->getCanvas()->drawPath(mMainPath, p);
}

void IGraphicsSkia::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
}

void IGraphicsSkia::LoadFont(const char* name)
{
}

void IGraphicsSkia::DrawBoxShadow(const IRECT& bounds, float cr, float ydrop, float pad, const IBlend* pBlend)
{
}

void IGraphicsSkia::PathTransformSetMatrix(const IMatrix& m)
{
}

void IGraphicsSkia::SetClipRegion(const IRECT& r)
{
}
