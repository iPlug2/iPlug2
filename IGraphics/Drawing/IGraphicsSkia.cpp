#include <cmath>

#include "IGraphicsSkia.h"

#ifdef OS_MAC
#include "SkCGUtils.h"
#endif

SkiaBitmap::SkiaBitmap(const char* path, double sourceScale)
{
  auto data = SkData::MakeFromFileName(path);
  mImage = SkImage::MakeFromEncoded(data);

  SetBitmap(mImage.get(), mImage->width(), mImage->height(), sourceScale);
}

#pragma mark -

// Utility conversions
inline SkColor SkiaColor(const IColor& color, const IBlend* pBlend = 0)
{
  return SkColorSetARGB(color.A, color.R, color.G, color.B);
}

inline SkRect ToSkiaRect(const IRECT& r)
{
  return SkRect::MakeLTRB(r.L, r.T, r.R, r.B);
}

inline SkBlendMode SkiaBlendMode(const IBlend* pBlend)
{
  switch (pBlend->mMethod)
  {
    case EBlendType::kBlendAdd: return SkBlendMode::kPlus;
    case EBlendType::kBlendClobber: return SkBlendMode::kClear;
    case EBlendType::kBlendColorDodge: return SkBlendMode::kColorDodge;
    case EBlendType::kBlendNone:
    default:
      return SkBlendMode::kClear;
  }
}

SkPaint SkiaPaint(const IPattern& pattern, const IBlend* pBlend)
{
  SkPaint paint;
  paint.setAntiAlias(true);
  
  if(pattern.mType == kSolidPattern)
    paint.setColor(SkiaColor(pattern.GetStop(0).mColor, pBlend));
//  else if (pattern.mType == kRadialPattern)
//  {
//  }
//  else
//  {
//  }
  
  return paint;
}

#pragma mark -

IGraphicsSkia::IGraphicsSkia(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
{
  DBGMSG("IGraphics Skia @ %i FPS\n", fps);
}

IGraphicsSkia::~IGraphicsSkia()
{
}

APIBitmap* IGraphicsSkia::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new SkiaBitmap(resourcePath.Get(), scale);
}

void IGraphicsSkia::SetPlatformContext(void* pContext)
{
  mPlatformContext = pContext;
  
  mSurface = SkSurface::MakeRasterN32Premul(WindowWidth() * GetDisplayScale(), WindowHeight() * GetDisplayScale());
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
  
//  mSurface = SkSurface::MakeRasterN32Premul(WindowWidth() * GetDisplayScale(), WindowHeight() * GetDisplayScale());
}

void IGraphicsSkia::BeginFrame()
{
  mSurface->getCanvas()->clear(SK_ColorWHITE);
}

void IGraphicsSkia::EndFrame()
{
#ifdef IGRAPHICS_CPU
  #ifdef OS_MAC
    SkPixmap pixmap;
    mSurface->peekPixels(&pixmap);
    SkBitmap bmp;
    bmp.installPixels(pixmap);
    SkCGDrawBitmap((CGContextRef) mPlatformContext, bmp, 0, 0);
  #endif
#endif
}

void IGraphicsSkia::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  SkPaint p;
  p.setFilterQuality(kHigh_SkFilterQuality);
  mSurface->getCanvas()->drawImage(dynamic_cast<SkImage*>(bitmap.GetAPIBitmap()), 0, 0, &p);
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
  SkPaint paint = SkiaPaint(pattern, pBlend);
  paint.setStyle(SkPaint::kStroke_Style);
  mSurface->getCanvas()->drawPath(mMainPath, paint);

  switch (options.mCapOption)
  {
    case kCapButt:   paint.setStrokeCap(SkPaint::kButt_Cap);     break;
    case kCapRound:  paint.setStrokeCap(SkPaint::kRound_Cap);    break;
    case kCapSquare: paint.setStrokeCap(SkPaint::kSquare_Cap);   break;
  }

  switch (options.mJoinOption)
  {
    case kJoinMiter: paint.setStrokeJoin(SkPaint::kMiter_Join);   break;
    case kJoinRound: paint.setStrokeJoin(SkPaint::kRound_Join);   break;
    case kJoinBevel: paint.setStrokeJoin(SkPaint::kBevel_Join);   break;
  }
  
  paint.setStrokeWidth(thickness);
  paint.setStrokeMiter(options.mMiterLimit);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsSkia::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  SkPaint paint = SkiaPaint(pattern, pBlend);
  paint.setStyle(SkPaint::kFill_Style);
  
  if (options.mFillRule == EFillRule::kFillWinding)
    mMainPath.setFillType(SkPath::kWinding_FillType);
  else
    mMainPath.setFillType(SkPath::kEvenOdd_FillType);
  
  mSurface->getCanvas()->drawPath(mMainPath, paint);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsSkia::LoadFont(const char* name)
{
}

void IGraphicsSkia::DrawBoxShadow(const IRECT& bounds, float cr, float ydrop, float pad, const IBlend* pBlend)
{
}

void IGraphicsSkia::PathTransformSetMatrix(const IMatrix& m)
{
  mSurface->getCanvas()->scale(GetScale(), GetScale());
  //TODO:
}

void IGraphicsSkia::SetClipRegion(const IRECT& r)
{
  //TODO:

//  SkRect skrect;
//  skrect.set(r.L, r.T, r.R, r.B);
//  mSurface->getCanvas()->clipRect(skrect);
}
