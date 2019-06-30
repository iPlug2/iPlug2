#include <cmath>

#include "IGraphicsSkia.h"

#include "SkDashPathEffect.h"
#include "SkGradientShader.h"
#include "SkFont.h"

#include "gl/GrGLInterface.h"
#include "gl/GrGLUtil.h"
#include "GrContext.h"

#ifdef OS_MAC
#include "SkCGUtils.h"
#endif

#include <OpenGL/gl.h>


SkiaBitmap::SkiaBitmap(int width, int height, int scale, float drawScale)
{
#ifdef GRAPHICS_GL
    mDrawable.mSurface = SkSurface::MakeFromBackendRenderTarget(mGrContext.get(), backendRenderTarget, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, nullptr);
#else
    mDrawable.mSurface = SkSurface::MakeRasterN32Premul(width, height);
#endif
    mDrawable.mIsSurface = true;
    
    SetBitmap(&mDrawable, width, height, scale, drawScale);
}

SkiaBitmap::SkiaBitmap(const char* path, double sourceScale)
{
  auto data = SkData::MakeFromFileName(path);
  mDrawable.mImage = SkImage::MakeFromEncoded(data);

  mDrawable.mIsSurface = false;
  SetBitmap(&mDrawable, mDrawable.mImage->width(), mDrawable.mImage->height(), sourceScale, 1.f);
}

#pragma mark -

// Utility conversions
inline SkColor SkiaColor(const IColor& color, const IBlend* pBlend = 0)
{
  return SkColorSetARGB(color.A, color.R, color.G, color.B);
}

inline SkRect SkiaRect(const IRECT& r)
{
  return SkRect::MakeLTRB(r.L, r.T, r.R, r.B);
}

inline SkBlendMode SkiaBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
    return SkBlendMode::kSrcOver;
    
  switch (pBlend->mMethod)
  {
    case EBlend::Default:         // fall through
    case EBlend::Clobber:         // fall through
    case EBlend::SourceOver:      return SkBlendMode::kSrcOver;
    case EBlend::SourceIn:        return SkBlendMode::kSrcIn;
    case EBlend::SourceOut:       return SkBlendMode::kSrcOut;
    case EBlend::SourceAtop:      return SkBlendMode::kSrcATop;
    case EBlend::DestOver:        return SkBlendMode::kDstOver;
    case EBlend::DestIn:          return SkBlendMode::kDstIn;
    case EBlend::DestOut:         return SkBlendMode::kDstOut;
    case EBlend::DestAtop:        return SkBlendMode::kDstATop;
    case EBlend::Add:             return SkBlendMode::kPlus;
    case EBlend::XOR:             return SkBlendMode::kXor;
  }
  
  return SkBlendMode::kClear;
}

SkPaint SkiaPaint(const IPattern& pattern, const IBlend* pBlend)
{
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setBlendMode(SkiaBlendMode(pBlend));
    
  if (pattern.mType == EPatternType::Solid)
    paint.setColor(SkiaColor(pattern.GetStop(0).mColor, pBlend));
  else
  {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 1.0;
      
    IMatrix m = pattern.mTransform;
    m.Invert();
    m.TransformPoint(x1, y1);
    m.TransformPoint(x2, y2);
      
    SkPoint points[2] =
    {
      SkPoint::Make(x1, y1),
      SkPoint::Make(x2, y2)
    };
    
    SkColor colors[8];
    
    assert(pattern.NStops() <= 8);
    
    for(int i = 0; i < pattern.NStops(); i++)
      colors[i] = SkiaColor(pattern.GetStop(i).mColor);
   
    if(pattern.mType == EPatternType::Linear)
      paint.setShader(SkGradientShader::MakeLinear(points, colors, nullptr, pattern.NStops(), SkTileMode::kClamp, 0, nullptr));
    else
    {
      float xd = points[0].x() - points[1].x();
      float yd = points[0].y() - points[1].y();
      float radius = std::sqrt(xd * xd + yd * yd);
        
      paint.setShader(SkGradientShader::MakeRadial(points[0], radius, colors, nullptr, pattern.NStops(), SkTileMode::kClamp, 0, nullptr));
    }
  }
  
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

bool IGraphicsSkia::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) /*|| (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr)*/;
}

APIBitmap* IGraphicsSkia::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
  return new SkiaBitmap(fileNameOrResID, scale);
}

void IGraphicsSkia::OnViewInitialized(void* pContext)
{
#if defined IGRAPHICS_GL
  int fbo = 0, samples = 0, stencilBits = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
  glGetIntegerv(GL_SAMPLES, &samples);
  glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
  
  auto interface = GrGLMakeNativeInterface();
  mGrContext = GrContext::MakeGL(interface);
  
  GrGLFramebufferInfo fbinfo;
  fbinfo.fFBOID = fbo;
  fbinfo.fFormat = 0x8058;
  
  auto backendRenderTarget = GrBackendRenderTarget(WindowWidth(), WindowHeight(), samples, stencilBits, fbinfo);

  mSurface = SkSurface::MakeFromBackendRenderTarget(mGrContext.get(), backendRenderTarget, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, nullptr);
#else
  mSurface = SkSurface::MakeRasterN32Premul(WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
#endif
  mCanvas = mSurface->getCanvas();
}

void IGraphicsSkia::OnViewDestroyed()
{
}

void IGraphicsSkia::DrawResize()
{
#if defined IGRAPHICS_CPU
   mSurface = SkSurface::MakeRasterN32Premul(WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
   mCanvas = mSurface->getCanvas();
#endif
}

void IGraphicsSkia::BeginFrame()
{
  mCanvas->clear(SK_ColorWHITE);
}

void IGraphicsSkia::EndFrame()
{
#ifdef IGRAPHICS_CPU
  #ifdef OS_MAC
    SkPixmap pixmap;
    mSurface->peekPixels(&pixmap);
    SkBitmap bmp;
    bmp.installPixels(pixmap);
    CGContext* pCGContext = (CGContextRef) mPlatformContext;
    CGContextSaveGState(pCGContext);
    CGContextScaleCTM(pCGContext, 1.0 / GetScreenScale(), 1.0 / GetScreenScale());
    SkCGDrawBitmap(pCGContext, bmp, 0, 0);
    CGContextRestoreGState(pCGContext);
  #endif
#else
  mCanvas->flush();
#endif
}

void IGraphicsSkia::DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  SkPaint p;
  p.setFilterQuality(kHigh_SkFilterQuality);
  p.setBlendMode(SkiaBlendMode(pBlend));
    
  SkiaDrawable* image = bitmap.GetAPIBitmap()->GetBitmap();

  double scale = 1.0 / (bitmap.GetScale() * bitmap.GetDrawScale());
    
  mCanvas->save();
  mCanvas->translate(dest.L, dest.T);
  mCanvas->scale(scale, scale);

  if (image->mIsSurface)
    image->mSurface->draw(mCanvas, 0.0, 0.0, &p);
  else
    mCanvas->drawImage(image->mImage, 0.0, 0.0, &p);
    
    mCanvas->restore();
}

IColor IGraphicsSkia::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

void IGraphicsSkia::DoMeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  
}

void IGraphicsSkia::DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
//  SkFont font;
//  font.setSubpixel(true);
//  font.setSize(text.mSize);
//  SkPaint paint;
//  paint.setColor(SkiaColor(text.mFGColor, pBlend));
//  
//  mCanvas->drawSimpleText(str, strlen(str), SkTextEncoding::kUTF8, bounds.L, bounds.T, font, paint);
}

void IGraphicsSkia::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  float dashArray[8];

  SkPaint paint = SkiaPaint(pattern, pBlend);
  paint.setStyle(SkPaint::kStroke_Style);

  switch (options.mCapOption)
  {
    case ELineCap::Butt:   paint.setStrokeCap(SkPaint::kButt_Cap);     break;
    case ELineCap::Round:  paint.setStrokeCap(SkPaint::kRound_Cap);    break;
    case ELineCap::Square: paint.setStrokeCap(SkPaint::kSquare_Cap);   break;
  }

  switch (options.mJoinOption)
  {
    case ELineJoin::Miter: paint.setStrokeJoin(SkPaint::kMiter_Join);   break;
    case ELineJoin::Round: paint.setStrokeJoin(SkPaint::kRound_Join);   break;
    case ELineJoin::Bevel: paint.setStrokeJoin(SkPaint::kBevel_Join);   break;
  }
  
  if(options.mDash.GetCount())
  {
    for (int i = 0; i < options.mDash.GetCount(); i++)
      dashArray[i] = *(options.mDash.GetArray() + i);
    
    paint.setPathEffect(SkDashPathEffect::Make(dashArray, options.mDash.GetCount(), 0));
  }
  
  paint.setStrokeWidth(thickness);
  paint.setStrokeMiter(options.mMiterLimit);
  
  mCanvas->drawPath(mMainPath, paint);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsSkia::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  SkPaint paint = SkiaPaint(pattern, pBlend);
  paint.setStyle(SkPaint::kFill_Style);
  
  if (options.mFillRule == EFillRule::Winding)
    mMainPath.setFillType(SkPath::kWinding_FillType);
  else
    mMainPath.setFillType(SkPath::kEvenOdd_FillType);
  
  mCanvas->drawPath(mMainPath, paint);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsSkia::PathTransformSetMatrix(const IMatrix& m)
{
  double xTranslate = 0.0;
  double yTranslate = 0.0;
    
  if (!mCanvas)
    return;
    
  if (!mLayers.empty())
  {
    IRECT bounds = mLayers.top()->Bounds();
    
    xTranslate = -bounds.L;
    yTranslate = -bounds.T;
  }
    
  SkMatrix globalMatix = SkMatrix::MakeScale(GetScreenScale() * GetDrawScale());
  SkMatrix skMatrix = SkMatrix::MakeAll(m.mXX, m.mYX, m.mTX, m.mXY, m.mYY, m.mTY, 0, 0, 1);
  globalMatix.preTranslate(xTranslate, yTranslate);
  skMatrix.postConcat(globalMatix);
  mCanvas->setMatrix(skMatrix);
}

void IGraphicsSkia::SetClipRegion(const IRECT& r)
{
  SkRect skrect;
  skrect.set(r.L, r.T, r.R, r.B);
  mCanvas->restore();
  mCanvas->save();
  mCanvas->clipRect(skrect);
}

APIBitmap* IGraphicsSkia::CreateAPIBitmap(int width, int height, int scale, double drawScale)
{
  return new SkiaBitmap(width, height, scale, drawScale);
}

void IGraphicsSkia::UpdateLayer()
{
    mCanvas = mLayers.empty() ? mSurface->getCanvas() : mLayers.top()->GetAPIBitmap()->GetBitmap()->mSurface->getCanvas();
}
