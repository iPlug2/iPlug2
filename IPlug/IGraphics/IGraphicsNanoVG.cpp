#include <cmath>

#include "IGraphicsNanoVG.h"
#include "IGraphicsNanoSVG.h"

#pragma mark -

inline int GetBitmapIdx(APIBitmap* pBitmap) { return (int) ((long long) pBitmap->GetBitmap()); }

NanoVGBitmap::NanoVGBitmap(NVGcontext* pContext, const char* path, double sourceScale)
{
  mVG = pContext;
  int w = 0, h = 0;
  long long idx = nvgCreateImage(mVG, path, 0);
  nvgImageSize(mVG, idx, &w, &h);
      
  SetBitmap((void*) idx, w, h, sourceScale);
}

NanoVGBitmap::~NanoVGBitmap()
{
  int idx = GetBitmapIdx(this);
  nvgDeleteImage(mVG, idx);
}
  
IGraphicsNanoVG::IGraphicsNanoVG(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
}

IGraphicsNanoVG::~IGraphicsNanoVG() 
{
  mBitmaps.Empty(true);
  
#ifdef OS_MAC
  if(mVG)
    nvgDeleteMTL(mVG);
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

IBitmap IGraphicsNanoVG::ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale)
{
  return bitmap;
}
/*
IBitmap IGraphicsNanoVG::CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, int targetScale)
{
  return bitmap;
}*/

void IGraphicsNanoVG::ViewInitialized(void* layer)
{
#ifdef OS_MAC
  mVG = nvgCreateMTL(layer, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
}

void IGraphicsNanoVG::BeginFrame()
{
  nvgBeginFrame(mVG, Width(), Height(), GetDisplayScale());
}

void IGraphicsNanoVG::EndFrame()
{
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

void IGraphicsNanoVG::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  int idx = GetBitmapIdx(bitmap.GetAPIBitmap());
  NVGpaint imgPaint = nvgImagePattern(mVG, destCtrX, destCtrY, bitmap.W(), bitmap.H(), angle, idx, BlendWeight(pBlend));
  PathClear();
  nvgRect(mVG, destCtrX, destCtrY, destCtrX + bitmap.W(), destCtrX + bitmap.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
  PathClear();
}

void IGraphicsNanoVG::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend)
{
  //TODO:
}

IColor IGraphicsNanoVG::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsNanoVG::DrawText(const IText& text, const char* str, IRECT& rect, bool measure)
{
  return true;
}

bool IGraphicsNanoVG::MeasureText(const IText& text, const char* str, IRECT& destRect)
{
  return DrawText(text, str, destRect, true);
}

void IGraphicsNanoVG::NVGSetStrokeOptions(const IStrokeOptions& options)
{
  switch (options.mCapOption)
  {
    case kCapButt:   nvgLineCap(mVG, NSVG_CAP_BUTT);     break;
    case kCapRound:  nvgLineCap(mVG, NSVG_CAP_ROUND);    break;
    case kCapSquare: nvgLineCap(mVG, NSVG_CAP_SQUARE);   break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter:   nvgLineJoin(mVG, NVG_MITER);   break;
    case kJoinRound:   nvgLineJoin(mVG, NVG_ROUND);   break;
    case kJoinBevel:   nvgLineJoin(mVG, NVG_BEVEL);   break;
  }
  
  nvgMiterLimit(mVG, options.mMiterLimit);
  
  // TODO Dash
}

void IGraphicsNanoVG::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  NVGSetStrokeOptions(options);
  nvgStrokeWidth(mVG, thickness);
  Stroke(pattern, pBlend, options.mPreserve);
  nvgStrokeWidth(mVG, 1.0);
  NVGSetStrokeOptions();
}

void IGraphicsNanoVG::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  nvgPathWinding(mVG, options.mFillRule == kFillWinding ? NVG_CCW : NVG_CW);
  
  if (pattern.mType == kSolidPattern)
    nvgFillColor(mVG, NanoVGColor(pattern.GetStop(0).mColor, pBlend));
  else
    nvgFillPaint(mVG, GetNVGPaint(pattern, pBlend));
  
  nvgFill(mVG);
  if (!options.mPreserve)
    PathClear();
}

NVGpaint IGraphicsNanoVG::GetNVGPaint(const IPattern& pattern, const IBlend* pBlend)
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
    return nvgRadialGradient(mVG, s[0], s[1], 0.0, inverse[0], icol, ocol);
  }
  else
  {
    float e[2];
    nvgTransformPoint(&e[0], &e[1], inverse, 1, 0);
    
    return nvgLinearGradient(mVG, s[0], s[1], e[0], e[1], icol, ocol);
  }
}

void IGraphicsNanoVG::Stroke(const IPattern& pattern, const IBlend* pBlend, bool preserve)
{
  if (pattern.mType == kSolidPattern)
    nvgStrokeColor(mVG, NanoVGColor(pattern.GetStop(0).mColor, pBlend));
  else
    nvgStrokePaint(mVG, GetNVGPaint(pattern, pBlend));
  
  nvgStroke(mVG);
  if (!preserve)
    PathClear();
}
