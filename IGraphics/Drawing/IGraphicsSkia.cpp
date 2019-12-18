#include <cmath>
#include <map>

#include "IGraphicsSkia.h"

#include "SkDashPathEffect.h"
#include "SkGradientShader.h"
#include "SkFont.h"
#include "SkFontMetrics.h"
#include "SkTypeface.h"

#include "GrContext.h"

#include "IGraphicsSkia_src.cpp"

#if defined OS_MAC || defined OS_IOS
  #include "SkCGUtils.h"
  #if defined IGRAPHICS_GL2
    #include <OpenGL/gl.h>
  #elif defined IGRAPHICS_GL3
    #include <OpenGL/gl3.h>
  #elif defined IGRAPHICS_METAL
  //even though this is a .cpp we are in an objc(pp) compilation unit
    #import <Metal/Metal.h>
    #import <QuartzCore/CAMetalLayer.h>
  #elif !defined IGRAPHICS_CPU
    #error Define either IGRAPHICS_GL2, IGRAPHICS_GL3, IGRAPHICS_METAL, or IGRAPHICS_CPU for IGRAPHICS_SKIA with OS_MAC
  #endif
#elif defined OS_WIN
  #pragma comment(lib, "libpng.lib")
  #pragma comment(lib, "zlib.lib")
  #pragma comment(lib, "skia.lib")
  #ifdef IGRAPHICS_GL
    #pragma comment(lib, "opengl32.lib")
  #endif
#endif

#if defined IGRAPHICS_GL
  #include "gl/GrGLInterface.h"
#endif

using namespace iplug;
using namespace igraphics;

extern std::map<std::string, MTLTexturePtr> gTextureMap;

#pragma mark - Private Classes and Structs

class IGraphicsSkia::Bitmap : public APIBitmap
{
public:
  Bitmap(GrContext* context, int width, int height, int scale, float drawScale);
  Bitmap(const char* path, double sourceScale);
  Bitmap(const void* pData, int size, double sourceScale);
  Bitmap(sk_sp<SkImage>, double sourceScale);

private:
  SkiaDrawable mDrawable;
};
  
IGraphicsSkia::Bitmap::Bitmap(GrContext* context, int width, int height, int scale, float drawScale)
{
#ifdef IGRAPHICS_GL
  SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
  mDrawable.mSurface = SkSurface::MakeRenderTarget(context, SkBudgeted::kYes, info);
#else
  mDrawable.mSurface = SkSurface::MakeRasterN32Premul(width, height);
#endif
  mDrawable.mIsSurface = true;
  
  SetBitmap(&mDrawable, width, height, scale, drawScale);
}

IGraphicsSkia::Bitmap::Bitmap(const char* path, double sourceScale)
{
  auto data = SkData::MakeFromFileName(path);
  mDrawable.mImage = SkImage::MakeFromEncoded(data);
  
  mDrawable.mIsSurface = false;
  SetBitmap(&mDrawable, mDrawable.mImage->width(), mDrawable.mImage->height(), sourceScale, 1.f);
}

IGraphicsSkia::Bitmap::Bitmap(const void* pData, int size, double sourceScale)
{
  auto data = SkData::MakeWithoutCopy(pData, size);
  mDrawable.mImage = SkImage::MakeFromEncoded(data);
  
  mDrawable.mIsSurface = false;
  SetBitmap(&mDrawable, mDrawable.mImage->width(), mDrawable.mImage->height(), sourceScale, 1.f);
}

IGraphicsSkia::Bitmap::Bitmap(sk_sp<SkImage> image, double sourceScale)
{
  mDrawable.mImage = image;
  SetBitmap(&mDrawable, mDrawable.mImage->width(), mDrawable.mImage->height(), sourceScale, 1.f);
}

struct IGraphicsSkia::Font
{
  Font(IFontDataPtr&& data, sk_sp<SkTypeface> typeFace)
    : mData(std::move(data)), mTypeface(typeFace) {}
    
  IFontDataPtr mData;
  sk_sp<SkTypeface> mTypeface;
};

// Fonts
StaticStorage<IGraphicsSkia::Font> IGraphicsSkia::sFontCache;

#pragma mark -

// Utility conversions
static inline SkColor SkiaColor(const IColor& color, const IBlend* pBlend)
{
  if (pBlend)
    return SkColorSetARGB(Clip(static_cast<int>(pBlend->mWeight * color.A), 0, 255), color.R, color.G, color.B);
  else
    return SkColorSetARGB(color.A, color.R, color.G, color.B);
}

static inline SkRect SkiaRect(const IRECT& r)
{
  return SkRect::MakeLTRB(r.L, r.T, r.R, r.B);
}

static inline SkBlendMode SkiaBlendMode(const IBlend* pBlend)
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

static inline SkTileMode SkiaTileMode(const IPattern& pattern)
{
  switch (pattern.mExtend)
  {
    case EPatternExtend::None:      return SkTileMode::kDecal;
    case EPatternExtend::Reflect:   return SkTileMode::kMirror;
    case EPatternExtend::Repeat:    return SkTileMode::kRepeat;
    case EPatternExtend::Pad:       return SkTileMode::kClamp;
  }

  return SkTileMode::kClamp;
}

static SkPaint SkiaPaint(const IPattern& pattern, const IBlend* pBlend)
{
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setBlendMode(SkiaBlendMode(pBlend));
    
  if (pattern.mType == EPatternType::Solid || pattern.NStops() <  2)
  {
    paint.setColor(SkiaColor(pattern.GetStop(0).mColor, pBlend));
  }
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
    SkScalar positions[8];
      
    assert(pattern.NStops() <= 8);
    
    for(int i = 0; i < pattern.NStops(); i++)
    {
      const IColorStop& stop = pattern.GetStop(i);
      colors[i] = SkiaColor(stop.mColor, pBlend);
      positions[i] = stop.mOffset;
    }
   
    if(pattern.mType == EPatternType::Linear)
      paint.setShader(SkGradientShader::MakeLinear(points, colors, positions, pattern.NStops(), SkiaTileMode(pattern), 0, nullptr));
    else
    {
      float xd = points[0].x() - points[1].x();
      float yd = points[0].y() - points[1].y();
      float radius = std::sqrt(xd * xd + yd * yd);
        
      paint.setShader(SkGradientShader::MakeRadial(points[0], radius, colors, positions, pattern.NStops(), SkiaTileMode(pattern), 0, nullptr));
    }
  }
    
  return paint;
}

#pragma mark -

IGraphicsSkia::IGraphicsSkia(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
{
  DBGMSG("IGraphics Skia @ %i FPS\n", fps);
  
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Retain();
}

IGraphicsSkia::~IGraphicsSkia()
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Release();
}

bool IGraphicsSkia::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) /*|| (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr)*/;
}

APIBitmap* IGraphicsSkia::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
#ifdef OS_IOS
  if (location == EResourceLocation::kPreloadedTexture)
  {
    GrMtlTextureInfo textureInfo;
    textureInfo.fTexture.retain((__bridge const void*)(gTextureMap[fileNameOrResID]));
    id<MTLTexture> texture = (id<MTLTexture>) textureInfo.fTexture.get();
    
    MTLPixelFormat pixelFormat = texture.pixelFormat;
    
    auto grBackendTexture = GrBackendTexture(texture.width, texture.height, GrMipMapped::kNo, textureInfo);
    
    sk_sp<SkImage> image = SkImage::MakeFromTexture(mGrContext.get(), grBackendTexture, kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType, kOpaque_SkAlphaType, nullptr);
    return new Bitmap(image, scale);
  }
  else
#endif
#ifdef OS_WIN
  if (location == EResourceLocation::kWinBinary)
  {
    int size = 0;
    const void* pData = LoadWinResource(fileNameOrResID, "png", size, GetWinModuleHandle());
    return new Bitmap(pData, size, scale);
  }
  else
#endif
  return new Bitmap(fileNameOrResID, scale);
}

void IGraphicsSkia::OnViewInitialized(void* pContext)
{
#if defined IGRAPHICS_GL
  auto glInterface = GrGLMakeNativeInterface();
  mGrContext = GrContext::MakeGL(glInterface);
#elif defined IGRAPHICS_METAL
  @autoreleasepool {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> commandQueue = [device newCommandQueue];
    mGrContext = GrContext::MakeMetal(device, commandQueue);
    mMTLDevice = device;
    mMTLCommandQueue = commandQueue;
    mMTLLayer = pContext;
    ((CAMetalLayer*) pContext).device = device;
  }
#endif
    
  DrawResize();
}

void IGraphicsSkia::OnViewDestroyed()
{
}

void IGraphicsSkia::DrawResize()
{
  auto w = WindowWidth() * GetScreenScale();
  auto h = WindowHeight() * GetScreenScale();
  
#if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
  if (mGrContext.get())
  {
    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    mSurface = SkSurface::MakeRenderTarget(mGrContext.get(), SkBudgeted::kYes, info);
  }
#else
  #ifdef OS_WIN
    mSurface.reset();
    const size_t bmpSize = sizeof(BITMAPINFOHEADER) + WindowWidth() * WindowHeight() * GetScreenScale() * sizeof(uint32_t);
    mSurfaceMemory.Resize(bmpSize);
    BITMAPINFO* bmpInfo = reinterpret_cast<BITMAPINFO*>(mSurfaceMemory.Get());
    ZeroMemory(bmpInfo, sizeof(BITMAPINFO));
    bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo->bmiHeader.biWidth = w;
    bmpInfo->bmiHeader.biHeight = -h; // negative means top-down bitmap. Skia draws top-down.
    bmpInfo->bmiHeader.biPlanes = 1;
    bmpInfo->bmiHeader.biBitCount = 32;
    bmpInfo->bmiHeader.biCompression = BI_RGB;
    void* pixels = bmpInfo->bmiColors;

    SkImageInfo info = SkImageInfo::Make(w, h, kN32_SkColorType, kPremul_SkAlphaType, nullptr);
    mSurface = SkSurface::MakeRasterDirect(info, pixels, sizeof(uint32_t) * w);
  #else
    mSurface = SkSurface::MakeRasterN32Premul(w, h);
  #endif
#endif
  if (mSurface)
    mCanvas = mSurface->getCanvas();
}

void IGraphicsSkia::BeginFrame()
{
  int width = WindowWidth() * GetScreenScale();
  int height = WindowHeight() * GetScreenScale();
  
#if defined IGRAPHICS_GL
  if (mGrContext.get())
  {
    // Bind to the current main framebuffer
    int fbo = 0, samples = 0, stencilBits = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    glGetIntegerv(GL_SAMPLES, &samples);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    
    GrGLFramebufferInfo fbinfo;
    fbinfo.fFBOID = fbo;
    fbinfo.fFormat = 0x8058;

    GrBackendRenderTarget backendRT(width, height, samples, stencilBits, fbinfo);
    
    mScreenSurface = SkSurface::MakeFromBackendRenderTarget(mGrContext.get(), backendRT, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, nullptr);
    assert(mScreenSurface);
  }
#elif defined IGRAPHICS_METAL
  if (mGrContext.get())
  {
    id<CAMetalDrawable> drawable = [(CAMetalLayer*) mMTLLayer nextDrawable];
    
    GrMtlTextureInfo fbInfo;
    fbInfo.fTexture.retain((__bridge const void*)(drawable.texture));
    GrBackendRenderTarget backendRT(width, height, 1 /* sample count/MSAA */, fbInfo);
    
    mScreenSurface = SkSurface::MakeFromBackendRenderTarget(mGrContext.get(), backendRT, kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType, nullptr, nullptr);
    
    mMTLDrawable = drawable;
  }
  assert(mScreenSurface);
#endif

  IGraphics::BeginFrame();
}

void IGraphicsSkia::EndFrame()
{
#ifdef IGRAPHICS_CPU
  #if defined OS_MAC || defined OS_IOS
    SkPixmap pixmap;
    mSurface->peekPixels(&pixmap);
    SkBitmap bmp;
    bmp.installPixels(pixmap);  
    CGContext* pCGContext = (CGContextRef) mPlatformContext;
    CGContextSaveGState(pCGContext);
    CGContextScaleCTM(pCGContext, 1.0 / GetScreenScale(), 1.0 / GetScreenScale());
    SkCGDrawBitmap(pCGContext, bmp, 0, 0);
    CGContextRestoreGState(pCGContext);
  #elif defined OS_WIN
    auto w = WindowWidth() * GetScreenScale();
    auto h = WindowHeight() * GetScreenScale();
    BITMAPINFO* bmpInfo = reinterpret_cast<BITMAPINFO*>(mSurfaceMemory.Get());
    HWND hWnd = (HWND) GetWindow();
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    StretchDIBits(hdc, 0, 0, w, h, 0, 0, w, h, bmpInfo->bmiColors, bmpInfo,  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hWnd, hdc);
    EndPaint(hWnd, &ps);
  #else
    #error NOT IMPLEMENTED
  #endif
#else
  mSurface->draw(mScreenSurface->getCanvas(), 0.0, 0.0, nullptr);
  mScreenSurface->getCanvas()->flush();
  
  #ifdef IGRAPHICS_METAL
    id<MTLCommandBuffer> commandBuffer = [(id<MTLCommandQueue>) mMTLCommandQueue commandBuffer];
    commandBuffer.label = @"Present";
  
    [commandBuffer presentDrawable:(id<CAMetalDrawable>) mMTLDrawable];
    [commandBuffer commit];
  #endif
#endif
}

void IGraphicsSkia::DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  SkPaint p;
  SkRect skrect;
  
  p.setFilterQuality(kHigh_SkFilterQuality);
  p.setBlendMode(SkiaBlendMode(pBlend));
  if (pBlend)
    p.setAlpha(Clip(static_cast<int>(pBlend->mWeight * 255), 0, 255));
    
  SkiaDrawable* image = bitmap.GetAPIBitmap()->GetBitmap();

  double scale1 = 1.0 / (bitmap.GetScale() * bitmap.GetDrawScale());
  double scale2 = bitmap.GetScale() * bitmap.GetDrawScale();
  
  mCanvas->save();
  skrect.setLTRB(dest.L, dest.T, dest.R, dest.B);
  mCanvas->clipRect(skrect);
  mCanvas->translate(dest.L, dest.T);
  mCanvas->scale(scale1, scale1);
  mCanvas->translate(-srcX * scale2, -srcY * scale2);
  
  if (image->mIsSurface)
    image->mSurface->draw(mCanvas, 0.0, 0.0, &p);
  else
    mCanvas->drawImage(image->mImage, 0.0, 0.0, &p);
    
  mCanvas->restore();
}

void IGraphicsSkia::PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding)
{
  SkPath arc;
  float sweep = (a2 - a1);

  if (sweep >= 360.f || sweep <= -360.f)
  {
    arc.addCircle(cx, cy, r);
    mMainPath.addPath(arc, mMatrix, SkPath::kAppend_AddPathMode);
  }
  else
  {
    if (winding == EWinding::CW)
    {
      while (sweep < 0)
        sweep += 360.f;
    }
    else
    {
      while (sweep > 0)
        sweep -= 360.f;
    }
      
    arc.arcTo(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r), a1 - 90.f, sweep, false);
    mMainPath.addPath(arc, mMatrix, SkPath::kExtend_AddPathMode);
  }
}

IColor IGraphicsSkia::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsSkia::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* cached = storage.Find(fontID);
  
  if (cached)
    return true;
  
  IFontDataPtr data = font->GetFontData();
  
  if (data->IsValid())
  {
    auto wrappedData = SkData::MakeWithoutCopy(data->Get(), data->GetSize());
    int index = data->GetFaceIdx();
    auto typeface = SkTypeface::MakeFromData(wrappedData, index);
    
    if (typeface)
    {
      storage.Add(new Font(std::move(data), typeface), fontID);
      return true;
    }
  }
  
  return false;
}

void IGraphicsSkia::PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y, SkFont& font) const
{
  SkFontMetrics metrics;
  SkPaint paint;
  SkRect bounds;
  
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* pFont = storage.Find(text.mFont);
  
  assert(pFont && "No font found - did you forget to load it?");

  font.setTypeface(pFont->mTypeface);
  font.setHinting(SkFontHinting::kNone);
  font.setForceAutoHinting(false);
  font.setSubpixel(true);
  font.setSize(text.mSize * pFont->mData->GetHeightEMRatio());
  
  // Draw / measure
  font.measureText(str, strlen(str), SkTextEncoding::kUTF8, &bounds);
  font.getMetrics(&metrics);
  
  const double textWidth = bounds.width();// + textExtents.x_bearing;
  const double textHeight = text.mSize;
  const double ascender = metrics.fAscent;
  const double descender = metrics.fDescent;
  
  switch (text.mAlign)
  {
    case EAlign::Near:     x = r.L;                          break;
    case EAlign::Center:   x = r.MW() - (textWidth / 2.0);   break;
    case EAlign::Far:      x = r.R - textWidth;              break;
  }
  
  switch (text.mVAlign)
  {
    case EVAlign::Top:      y = r.T - ascender;                            break;
    case EVAlign::Middle:   y = r.MH() - descender + (textHeight / 2.0);   break;
    case EVAlign::Bottom:   y = r.B - descender;                           break;
  }
  
  r = IRECT((float) x, (float) y + ascender, (float) (x + textWidth), (float) (y + ascender + textHeight));
}

void IGraphicsSkia::DoMeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  SkFont font;

  IRECT r = bounds;
  double x, y;
  PrepareAndMeasureText(text, str, bounds, x, y, font);
  DoMeasureTextRotation(text, r, bounds);
}

void IGraphicsSkia::DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  IRECT measured = bounds;
  
  SkFont font;
  double x, y;
  
  PrepareAndMeasureText(text, str, measured, x, y, font);
  PathTransformSave();
  DoTextRotation(text, bounds, measured);
  SkPaint paint;
  paint.setColor(SkiaColor(text.mFGColor, pBlend));
  mCanvas->drawSimpleText(str, strlen(str), SkTextEncoding::kUTF8, x, y, font, paint);
  PathTransformRestore();
}

void IGraphicsSkia::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
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
  
  if (options.mDash.GetCount())
  {
    // N.B. support odd counts by reading the array twice
    int dashCount = options.mDash.GetCount();
    int dashMax = dashCount & 1 ? dashCount * 2 : dashCount;
    float dashArray[16];
      
    for (int i = 0; i < dashMax; i += 2)
    {
      dashArray[i + 0] = options.mDash.GetArray()[i % dashCount];
      dashArray[i + 1] = options.mDash.GetArray()[(i + 1) % dashCount];
    }
    
    paint.setPathEffect(SkDashPathEffect::Make(dashArray, dashMax, 0));
  }
  
  paint.setStrokeWidth(thickness);
  paint.setStrokeMiter(options.mMiterLimit);
    
  RenderPath(paint);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsSkia::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  SkPaint paint = SkiaPaint(pattern, pBlend);
  paint.setStyle(SkPaint::kFill_Style);
  
  if (options.mFillRule == EFillRule::Winding)
    mMainPath.setFillType(SkPathFillType::kWinding);
  else
    mMainPath.setFillType(SkPathFillType::kEvenOdd);
  
  RenderPath(paint);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsSkia::RenderPath(SkPaint& paint)
{
  SkMatrix invMatrix;
    
  if (!mMatrix.isIdentity() && mMatrix.invert(&invMatrix))
  {
    SkPath path;
    mMainPath.transform(invMatrix, &path);
    mCanvas->drawPath(path, paint);
  }
  else
  {
    mCanvas->drawPath(mMainPath, paint);
  }
}

void IGraphicsSkia::PathTransformSetMatrix(const IMatrix& m)
{
  double scale = GetScreenScale() * GetDrawScale();
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

  mMatrix = SkMatrix::MakeAll(m.mXX, m.mXY, m.mTX, m.mYX, m.mYY, m.mTY, 0, 0, 1);
  SkMatrix globalMatrix = SkMatrix::MakeScale(scale);
  SkMatrix skMatrix = mMatrix;
  globalMatrix.preTranslate(xTranslate, yTranslate);
  skMatrix.postConcat(globalMatrix);
  mCanvas->setMatrix(skMatrix);
}

void IGraphicsSkia::SetClipRegion(const IRECT& r)
{
  SkRect skrect;
  skrect.setLTRB(r.L, r.T, r.R, r.B);
  mCanvas->restore();
  mCanvas->save();
  mCanvas->clipRect(skrect);
}

APIBitmap* IGraphicsSkia::CreateAPIBitmap(int width, int height, int scale, double drawScale)
{
  return new Bitmap(mGrContext.get(), width, height, scale, drawScale);
}

void IGraphicsSkia::UpdateLayer()
{
  mCanvas = mLayers.empty() ? mSurface->getCanvas() : mLayers.top()->GetAPIBitmap()->GetBitmap()->mSurface->getCanvas();
}

static size_t CalcRowBytes(int width)
{
  width = ((width + 7) & (-8));
  return width * sizeof(uint32_t);
}

void IGraphicsSkia::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  SkiaDrawable* pDrawable = layer->GetAPIBitmap()->GetBitmap();
  size_t rowBytes = CalcRowBytes(pDrawable->mSurface->width());
  int size = pDrawable->mSurface->height() * static_cast<int>(rowBytes);
    
  data.Resize(size);
   
  if (data.GetSize() >= size)
  {
    SkImageInfo info = SkImageInfo::MakeN32Premul(pDrawable->mSurface->width(), pDrawable->mSurface->height());
    pDrawable->mSurface->readPixels(info, data.Get(), rowBytes, 0, 0);
  }
}

void IGraphicsSkia::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  SkiaDrawable* pDrawable = layer->GetAPIBitmap()->GetBitmap();
  int width = pDrawable->mSurface->width();
  int height = pDrawable->mSurface->height();
  size_t rowBytes = CalcRowBytes(width);
  double scale = layer->GetAPIBitmap()->GetDrawScale() * layer->GetAPIBitmap()->GetScale();
  
  SkCanvas* pCanvas = pDrawable->mSurface->getCanvas();
    
  SkMatrix m;
  m.reset();
    
  SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
  SkPixmap pixMap(info, mask.Get(), rowBytes);
  sk_sp<SkImage> image = SkImage::MakeFromRaster(pixMap, nullptr, nullptr);
  sk_sp<SkImage> foreground;
    
  // Copy the foreground if needed
    
  if (shadow.mDrawForeground)
    foreground = pDrawable->mSurface->makeImageSnapshot();
 
  pCanvas->clear(SK_ColorTRANSPARENT);
 
  IBlend blend(EBlend::Default, shadow.mOpacity);
  pCanvas->setMatrix(m);
  pCanvas->drawImage(image.get(), shadow.mXOffset * scale, shadow.mYOffset * scale);
  m = SkMatrix::MakeScale(scale);
  pCanvas->setMatrix(m);
  pCanvas->translate(-layer->Bounds().L, -layer->Bounds().T);
  SkPaint p = SkiaPaint(shadow.mPattern, &blend);
  p.setBlendMode(SkBlendMode::kSrcIn);
  pCanvas->drawPaint(p);

  if (shadow.mDrawForeground)
  {
    m.reset();
    pCanvas->setMatrix(m);
    pCanvas->drawImage(foreground.get(), 0.0, 0.0);
  }
}

const char* IGraphicsSkia::GetDrawingAPIStr()
{
#ifdef IGRAPHICS_CPU
  return "SKIA | CPU";
#elif defined IGRAPHICS_GL2
  return "SKIA | OpenGL2";
#elif defined IGRAPHICS_GL3
  return "SKIA | OpenGL3";
#elif defined IGRAPHICS_METAL
  return "SKIA | Metal";
#endif
}
