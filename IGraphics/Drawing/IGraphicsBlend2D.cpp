/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cmath>
#include <map>

#include "IGraphicsBlend2D.h"

#if defined OS_MAC || defined OS_IOS
  #include <CoreGraphics/CoreGraphics.h>
#elif defined OS_WIN
  #include <windows.h>
#endif

using namespace iplug;
using namespace igraphics;

#pragma mark - Private Classes and Structs

class IGraphicsBlend2D::Bitmap : public APIBitmap
{
public:
  Bitmap(BLImage image, int width, int height, float scale, float drawScale);
  Bitmap(const char* path, double sourceScale);
  Bitmap(const void* pData, int size, double sourceScale);

  BLImage& GetImage() { return mImage; }
  const BLImage& GetImage() const { return mImage; }

private:
  BLImage mImage;
};

IGraphicsBlend2D::Bitmap::Bitmap(BLImage image, int width, int height, float scale, float drawScale)
  : mImage(std::move(image))
{
  SetBitmap(&mImage, width, height, scale, drawScale);
}

IGraphicsBlend2D::Bitmap::Bitmap(const char* path, double sourceScale)
{
  BLResult result = mImage.readFromFile(path);

  if (result != BL_SUCCESS)
  {
    assert(false && "Unable to load file at path");
  }

  SetBitmap(&mImage, mImage.width(), mImage.height(), sourceScale, 1.f);
}

IGraphicsBlend2D::Bitmap::Bitmap(const void* pData, int size, double sourceScale)
{
  BLArray<uint8_t> buffer;
  buffer.assignData(static_cast<const uint8_t*>(pData), size);
  BLResult result = mImage.readFromData(buffer);

  if (result != BL_SUCCESS)
  {
    assert(false && "Unable to load image from data");
  }

  SetBitmap(&mImage, mImage.width(), mImage.height(), sourceScale, 1.f);
}

struct IGraphicsBlend2D::Font
{
  Font(IFontDataPtr&& data, BLFontFace fontFace)
    : mData(std::move(data)), mFontFace(std::move(fontFace)) {}

  IFontDataPtr mData;
  BLFontFace mFontFace;
};

// Fonts
StaticStorage<IGraphicsBlend2D::Font> IGraphicsBlend2D::sFontCache;

#pragma mark - Utility conversions

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

BLRgba32 Blend2DColor(const IColor& color, const IBlend* pBlend)
{
  if (pBlend)
    return BLRgba32(color.R, color.G, color.B, Clip(static_cast<int>(pBlend->mWeight * color.A), 0, 255));
  else
    return BLRgba32(color.R, color.G, color.B, color.A);
}

BLRect Blend2DRect(const IRECT& r)
{
  return BLRect(r.L, r.T, r.W(), r.H());
}

BLCompOp Blend2DCompOp(const IBlend* pBlend)
{
  if (!pBlend)
    return BL_COMP_OP_SRC_OVER;

  switch (pBlend->mMethod)
  {
    case EBlend::SrcOver:      return BL_COMP_OP_SRC_OVER;
    case EBlend::SrcIn:        return BL_COMP_OP_SRC_IN;
    case EBlend::SrcOut:       return BL_COMP_OP_SRC_OUT;
    case EBlend::SrcAtop:      return BL_COMP_OP_SRC_ATOP;
    case EBlend::DstOver:      return BL_COMP_OP_DST_OVER;
    case EBlend::DstIn:        return BL_COMP_OP_DST_IN;
    case EBlend::DstOut:       return BL_COMP_OP_DST_OUT;
    case EBlend::DstAtop:      return BL_COMP_OP_DST_ATOP;
    case EBlend::Add:          return BL_COMP_OP_PLUS;
    case EBlend::XOR:          return BL_COMP_OP_XOR;
  }

  return BL_COMP_OP_SRC_OVER;
}

BLExtendMode Blend2DExtendMode(const IPattern& pattern)
{
  switch (pattern.mExtend)
  {
    case EPatternExtend::None:      return BL_EXTEND_MODE_PAD;
    case EPatternExtend::Reflect:   return BL_EXTEND_MODE_REFLECT;
    case EPatternExtend::Repeat:    return BL_EXTEND_MODE_REPEAT;
    case EPatternExtend::Pad:       return BL_EXTEND_MODE_PAD;
  }

  return BL_EXTEND_MODE_PAD;
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#pragma mark -

IGraphicsBlend2D::IGraphicsBlend2D(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphics(dlg, w, h, fps, scale)
{
  DBGMSG("IGraphics Blend2D CPU @ %i FPS\n", fps);
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Retain();
}

IGraphicsBlend2D::~IGraphicsBlend2D()
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Release();
}

bool IGraphicsBlend2D::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) ||
         (strstr(extLower, "jpg") != nullptr) ||
         (strstr(extLower, "jpeg") != nullptr) ||
         (strstr(extLower, "bmp") != nullptr) ||
         (strstr(extLower, "qoi") != nullptr);
}

APIBitmap* IGraphicsBlend2D::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
#ifdef OS_WIN
  if (location == EResourceLocation::kWinBinary)
  {
    int size = 0;
    const void* pData = LoadWinResource(fileNameOrResID, ext, size, GetWinModuleHandle());
    return new Bitmap(pData, size, scale);
  }
  else
#endif
  return new Bitmap(fileNameOrResID, scale);
}

APIBitmap* IGraphicsBlend2D::LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale)
{
  return new Bitmap(pData, dataSize, scale);
}

void IGraphicsBlend2D::OnViewInitialized(void* pContext)
{
  DrawResize();
}

void IGraphicsBlend2D::OnViewDestroyed()
{
  RemoveAllControls();
  mContext.end();
}

void IGraphicsBlend2D::DrawResize()
{
  auto w = static_cast<int>(std::ceil(static_cast<float>(WindowWidth()) * GetScreenScale()));
  auto h = static_cast<int>(std::ceil(static_cast<float>(WindowHeight()) * GetScreenScale()));

#ifdef OS_WIN
  const size_t bmpSize = w * h * 4;
  mSurfaceMemory.Resize(bmpSize);

  BLImageData imageData;
  imageData.pixelData = mSurfaceMemory.Get();
  imageData.stride = w * 4;
  imageData.size.w = w;
  imageData.size.h = h;
  imageData.format = BL_FORMAT_PRGB32;

  mSurface.createFromData(w, h, BL_FORMAT_PRGB32, mSurfaceMemory.Get(), w * 4);
#else
  mSurface.create(w, h, BL_FORMAT_PRGB32);
#endif

  SetupContext(mContext, mSurface);
}

void IGraphicsBlend2D::SetupContext(BLContext& ctx, BLImage& img)
{
  ctx.end();

  BLContextCreateInfo createInfo{};
  createInfo.threadCount = 0; // Use single thread for now

  ctx.begin(img, createInfo);
  ctx.setCompOp(BL_COMP_OP_SRC_OVER);
}

void IGraphicsBlend2D::BeginFrame()
{
  IGraphics::BeginFrame();

  // Clear the surface
  mContext.setCompOp(BL_COMP_OP_CLEAR);
  mContext.fillAll();
  mContext.setCompOp(BL_COMP_OP_SRC_OVER);
}

void IGraphicsBlend2D::EndFrame()
{
  mContext.flush(BL_CONTEXT_FLUSH_SYNC);

#if defined OS_MAC || defined OS_IOS
  BLImageData imageData;
  mSurface.getData(&imageData);

  CGContext* pCGContext = (CGContextRef) GetPlatformContext();
  if (pCGContext)
  {
    CGContextSaveGState(pCGContext);
    CGContextScaleCTM(pCGContext, 1.0 / GetScreenScale(), 1.0 / GetScreenScale());

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef bitmapContext = CGBitmapContextCreate(
      imageData.pixelData,
      imageData.size.w,
      imageData.size.h,
      8,
      imageData.stride,
      colorSpace,
      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    );

    if (bitmapContext)
    {
      CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
      if (cgImage)
      {
        // Flip vertically since CG has origin at bottom-left
        CGContextTranslateCTM(pCGContext, 0, imageData.size.h);
        CGContextScaleCTM(pCGContext, 1.0, -1.0);
        CGContextDrawImage(pCGContext, CGRectMake(0, 0, imageData.size.w, imageData.size.h), cgImage);
        CGImageRelease(cgImage);
      }
      CGContextRelease(bitmapContext);
    }
    CGColorSpaceRelease(colorSpace);

    CGContextRestoreGState(pCGContext);
  }
#elif defined OS_WIN
  auto w = static_cast<int>(WindowWidth() * GetScreenScale());
  auto h = static_cast<int>(WindowHeight() * GetScreenScale());

  BITMAPINFO bmpInfo;
  ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
  bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmpInfo.bmiHeader.biWidth = w;
  bmpInfo.bmiHeader.biHeight = -h; // negative means top-down bitmap
  bmpInfo.bmiHeader.biPlanes = 1;
  bmpInfo.bmiHeader.biBitCount = 32;
  bmpInfo.bmiHeader.biCompression = BI_RGB;

  HWND hWnd = (HWND) GetWindow();
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);
  StretchDIBits(hdc, 0, 0, w, h, 0, 0, w, h, mSurfaceMemory.Get(), &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
  ReleaseDC(hWnd, hdc);
  EndPaint(hWnd, &ps);
#endif
}

void IGraphicsBlend2D::DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  Bitmap* pAPIBitmap = static_cast<Bitmap*>(bitmap.GetAPIBitmap());

  if (!pAPIBitmap)
    return;

  BLImage& image = pAPIBitmap->GetImage();

  double scale1 = 1.0 / (bitmap.GetScale() * bitmap.GetDrawScale());
  double scale2 = bitmap.GetScale() * bitmap.GetDrawScale();

  mContext.save();

  // Set clipping
  mContext.clipToRect(Blend2DRect(dest));

  // Set blend mode and opacity
  mContext.setCompOp(Blend2DCompOp(pBlend));
  if (pBlend)
    mContext.setGlobalAlpha(pBlend->mWeight);

  // Calculate destination position
  double x = dest.L - srcX * scale1;
  double y = dest.T - srcY * scale1;

  // Apply scaling and draw
  mContext.translate(dest.L, dest.T);
  mContext.scale(scale1, scale1);
  mContext.translate(-srcX * scale2, -srcY * scale2);

  mContext.blitImage(BLPointI(0, 0), image);

  mContext.restore();

  // Reset blend mode
  mContext.setCompOp(BL_COMP_OP_SRC_OVER);
  mContext.setGlobalAlpha(1.0);
}

void IGraphicsBlend2D::PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding)
{
  float sweep = (a2 - a1);

  if (sweep >= 360.f || sweep <= -360.f)
  {
    // Full circle
    BLPath arc;
    arc.addCircle(BLCircle(cx, cy, r));

    BLMatrix2D m = BLMatrix2D::makeIdentity();
    m.transform(mMatrix);
    arc.transform(m);

    mMainPath.addPath(arc);
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

    // Convert to radians, adjust for Blend2D's coordinate system
    double startAngle = (a1 - 90.0) * (3.14159265358979323846 / 180.0);
    double sweepAngle = sweep * (3.14159265358979323846 / 180.0);

    // Calculate start and end points
    double startX = cx + r * std::cos(startAngle);
    double startY = cy + r * std::sin(startAngle);

    // Transform points through matrix
    BLPoint startPt(startX, startY);
    BLPoint centerPt(cx, cy);

    mMatrix.mapPoints(&startPt, &startPt, 1);
    mMatrix.mapPoints(&centerPt, &centerPt, 1);

    // Calculate transformed radius (assuming uniform scaling for simplicity)
    double sx = mMatrix.m00;
    double sy = mMatrix.m11;
    double transformedR = r * std::sqrt((sx * sx + sy * sy) / 2.0);

    mMainPath.arcTo(centerPt.x, centerPt.y, transformedR, startAngle, sweepAngle, false);
  }
}

void IGraphicsBlend2D::PathMoveTo(float x, float y)
{
  BLPoint pt(x, y);
  mMatrix.mapPoints(&pt, &pt, 1);
  mMainPath.moveTo(pt);
}

void IGraphicsBlend2D::PathLineTo(float x, float y)
{
  BLPoint pt(x, y);
  mMatrix.mapPoints(&pt, &pt, 1);
  mMainPath.lineTo(pt);
}

void IGraphicsBlend2D::PathCubicBezierTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
  BLPoint pts[3] = { BLPoint(x1, y1), BLPoint(x2, y2), BLPoint(x3, y3) };
  mMatrix.mapPoints(pts, pts, 3);
  mMainPath.cubicTo(pts[0], pts[1], pts[2]);
}

void IGraphicsBlend2D::PathQuadraticBezierTo(float cx, float cy, float x2, float y2)
{
  BLPoint pts[2] = { BLPoint(cx, cy), BLPoint(x2, y2) };
  mMatrix.mapPoints(pts, pts, 2);
  mMainPath.quadTo(pts[0], pts[1]);
}

IColor IGraphicsBlend2D::GetPoint(int x, int y)
{
  BLImageData imageData;
  mSurface.getData(&imageData);

  if (x < 0 || y < 0 || x >= imageData.size.w || y >= imageData.size.h)
    return COLOR_BLACK;

  uint8_t* pixels = static_cast<uint8_t*>(imageData.pixelData);
  int offset = (y * imageData.stride) + (x * 4);

  // PRGB32 format: B G R A
  uint8_t b = pixels[offset + 0];
  uint8_t g = pixels[offset + 1];
  uint8_t r = pixels[offset + 2];
  uint8_t a = pixels[offset + 3];

  return IColor(a, r, g, b);
}

bool IGraphicsBlend2D::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* cached = storage.Find(fontID);

  if (cached)
    return true;

  IFontDataPtr data = font->GetFontData();

  if (data->IsValid())
  {
    BLFontFace fontFace;
    BLFontData fontData;

    BLArray<uint8_t> buffer;
    buffer.assignData(data->Get(), data->GetSize());

    BLResult result = fontData.createFromData(buffer);
    if (result == BL_SUCCESS)
    {
      result = fontFace.createFromData(fontData, data->GetFaceIdx());
      if (result == BL_SUCCESS)
      {
        storage.Add(new Font(std::move(data), std::move(fontFace)), fontID);
        return true;
      }
    }
  }

  return false;
}

void IGraphicsBlend2D::PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double& y, BLFont& font) const
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* pFont = storage.Find(text.mFont);

  assert(pFont && "No font found - did you forget to load it?");

  font.createFromFace(pFont->mFontFace, text.mSize * pFont->mData->GetHeightEMRatio());

  BLFontMetrics fm = font.metrics();
  BLTextMetrics tm;
  BLGlyphBuffer gb;

  gb.setUtf8Text(str);
  font.shape(gb);
  font.getTextMetrics(gb, tm);

  double textWidth = tm.boundingBox.x1 - tm.boundingBox.x0;
  double textHeight = text.mSize;
  double ascender = fm.ascent;
  double descender = fm.descent;

  switch (text.mAlign)
  {
    case EAlign::Near:     x = r.L;                          break;
    case EAlign::Center:   x = r.MW() - (textWidth / 2.0);   break;
    case EAlign::Far:      x = r.R - textWidth;              break;
  }

  switch (text.mVAlign)
  {
    case EVAlign::Top:      y = r.T + ascender;                            break;
    case EVAlign::Middle:   y = r.MH() + (ascender - descender) / 2.0;     break;
    case EVAlign::Bottom:   y = r.B - descender;                           break;
  }

  r = IRECT((float) x, (float) (y - ascender), (float) (x + textWidth), (float) (y - ascender + textHeight));
}

float IGraphicsBlend2D::DoMeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  BLFont font;
  IRECT r = bounds;
  double x, y;
  PrepareAndMeasureText(text, str, bounds, x, y, font);
  DoMeasureTextRotation(text, r, bounds);
  return bounds.W();
}

void IGraphicsBlend2D::DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  IRECT measured = bounds;

  BLFont font;
  double x, y;

  PrepareAndMeasureText(text, str, measured, x, y, font);
  PathTransformSave();
  DoTextRotation(text, bounds, measured);

  mContext.setFillStyle(Blend2DColor(text.mFGColor, pBlend));
  mContext.setCompOp(Blend2DCompOp(pBlend));
  mContext.fillUtf8Text(BLPoint(x, y), font, str);
  mContext.setCompOp(BL_COMP_OP_SRC_OVER);

  PathTransformRestore();
}

BLGradient IGraphicsBlend2D::CreateGradient(const IPattern& pattern, const IBlend* pBlend)
{
  BLGradient gradient;

  int numStops = pattern.NStops();

  if (numStops < 2)
    return gradient;

  double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 1.0;

  IMatrix m = pattern.mTransform;
  m.Invert();
  m.TransformPoint(x1, y1);
  m.TransformPoint(x2, y2);

  switch (pattern.mType)
  {
    case EPatternType::Linear:
      gradient = BLGradient(BLLinearGradientValues(x1, y1, x2, y2));
      break;

    case EPatternType::Radial:
    {
      float xd = x1 - x2;
      float yd = y1 - y2;
      float radius = std::sqrt(xd * xd + yd * yd);
      gradient = BLGradient(BLRadialGradientValues(x1, y1, x1, y1, radius));
      break;
    }

    case EPatternType::Sweep:
      gradient = BLGradient(BLConicGradientValues(x1, y1, 0.0));
      break;

    default:
      return gradient;
  }

  gradient.setExtendMode(Blend2DExtendMode(pattern));

  for (int i = 0; i < numStops; i++)
  {
    const IColorStop& stop = pattern.GetStop(i);
    gradient.addStop(stop.mOffset, Blend2DColor(stop.mColor, pBlend));
  }

  return gradient;
}

void IGraphicsBlend2D::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  mContext.save();

  // Set stroke options
  BLStrokeOptions strokeOptions;
  strokeOptions.width = thickness;
  strokeOptions.miterLimit = options.mMiterLimit;

  switch (options.mCapOption)
  {
    case ELineCap::Butt:   strokeOptions.startCap = strokeOptions.endCap = BL_STROKE_CAP_BUTT;     break;
    case ELineCap::Round:  strokeOptions.startCap = strokeOptions.endCap = BL_STROKE_CAP_ROUND;    break;
    case ELineCap::Square: strokeOptions.startCap = strokeOptions.endCap = BL_STROKE_CAP_SQUARE;   break;
  }

  switch (options.mJoinOption)
  {
    case ELineJoin::Miter: strokeOptions.join = BL_STROKE_JOIN_MITER_CLIP;   break;
    case ELineJoin::Round: strokeOptions.join = BL_STROKE_JOIN_ROUND;        break;
    case ELineJoin::Bevel: strokeOptions.join = BL_STROKE_JOIN_BEVEL;        break;
  }

  mContext.setStrokeOptions(strokeOptions);

  // Handle dash pattern
  if (options.mDash.GetCount() > 0)
  {
    BLArray<double> dashes;
    int dashCount = options.mDash.GetCount();
    const float* dashArray = options.mDash.GetArray();

    for (int i = 0; i < dashCount; i++)
    {
      dashes.append(static_cast<double>(dashArray[i]));
    }

    mContext.setStrokeDashArray(dashes);
    mContext.setStrokeDashOffset(options.mDash.GetOffset());
  }

  // Set style
  mContext.setCompOp(Blend2DCompOp(pBlend));

  if (pattern.mType == EPatternType::Solid || pattern.NStops() < 2)
  {
    mContext.setStrokeStyle(Blend2DColor(pattern.GetStop(0).mColor, pBlend));
  }
  else
  {
    mContext.setStrokeStyle(CreateGradient(pattern, pBlend));
  }

  mContext.strokePath(mMainPath);
  mContext.restore();

  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsBlend2D::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  mContext.save();

  // Set fill rule
  if (options.mFillRule == EFillRule::Winding)
    mContext.setFillRule(BL_FILL_RULE_NON_ZERO);
  else
    mContext.setFillRule(BL_FILL_RULE_EVEN_ODD);

  // Set style
  mContext.setCompOp(Blend2DCompOp(pBlend));

  if (pattern.mType == EPatternType::Solid || pattern.NStops() < 2)
  {
    mContext.setFillStyle(Blend2DColor(pattern.GetStop(0).mColor, pBlend));
  }
  else
  {
    mContext.setFillStyle(CreateGradient(pattern, pBlend));
  }

  mContext.fillPath(mMainPath);
  mContext.restore();

  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsBlend2D::PathTransformSetMatrix(const IMatrix& m)
{
  double xTranslate = 0.0;
  double yTranslate = 0.0;

  if (!mLayers.empty())
  {
    IRECT bounds = mLayers.top()->Bounds();

    xTranslate = -bounds.L;
    yTranslate = -bounds.T;
  }

  mMatrix = BLMatrix2D::makeIdentity();
  mMatrix.postTranslate(xTranslate, yTranslate);
  mMatrix.postScale(GetTotalScale(), GetTotalScale());
  mMatrix.postTransform(BLMatrix2D(m.mXX, m.mYX, m.mXY, m.mYY, m.mTX, m.mTY));

  mClipMatrix = BLMatrix2D::makeIdentity();
  mClipMatrix.postTranslate(xTranslate, yTranslate);
  mClipMatrix.postScale(GetTotalScale(), GetTotalScale());

  mContext.setMatrix(mMatrix);
}

void IGraphicsBlend2D::SetClipRegion(const IRECT& r)
{
  mContext.restoreClipping();
  mContext.setMatrix(mClipMatrix);
  mContext.clipToRect(Blend2DRect(r));
  mContext.setMatrix(mMatrix);
}

APIBitmap* IGraphicsBlend2D::CreateAPIBitmap(int width, int height, float scale, double drawScale, bool cacheable)
{
  BLImage image;
  image.create(width, height, BL_FORMAT_PRGB32);

  // Clear the image
  BLContext ctx;
  ctx.begin(image);
  ctx.setCompOp(BL_COMP_OP_CLEAR);
  ctx.fillAll();
  ctx.end();

  return new Bitmap(std::move(image), width, height, scale, drawScale);
}

void IGraphicsBlend2D::UpdateLayer()
{
  if (mLayers.empty())
  {
    mContext.end();
    SetupContext(mContext, mSurface);
  }
  else
  {
    mContext.end();
    Bitmap* pBitmap = static_cast<Bitmap*>(mLayers.top()->GetAPIBitmap());
    SetupContext(mContext, pBitmap->GetImage());
  }
}

static size_t CalcRowBytes(int width)
{
  return width * sizeof(uint32_t);
}

void IGraphicsBlend2D::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  Bitmap* pBitmap = static_cast<Bitmap*>(layer->GetAPIBitmap());
  BLImageData imageData;
  pBitmap->GetImage().getData(&imageData);

  size_t rowBytes = CalcRowBytes(imageData.size.w);
  int size = imageData.size.h * static_cast<int>(rowBytes);

  data.Resize(size);

  if (data.GetSize() >= size)
  {
    // Copy row by row in case strides differ
    uint8_t* src = static_cast<uint8_t*>(imageData.pixelData);
    uint8_t* dst = data.Get();

    for (int y = 0; y < imageData.size.h; y++)
    {
      memcpy(dst + y * rowBytes, src + y * imageData.stride, rowBytes);
    }
  }
}

void IGraphicsBlend2D::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  Bitmap* pBitmap = static_cast<Bitmap*>(layer->GetAPIBitmap());
  BLImage& image = pBitmap->GetImage();
  BLImageData imageData;
  image.getData(&imageData);

  int width = imageData.size.w;
  int height = imageData.size.h;
  size_t rowBytes = CalcRowBytes(width);
  double scale = layer->GetAPIBitmap()->GetDrawScale() * layer->GetAPIBitmap()->GetScale();

  BLContext ctx;
  ctx.begin(image);

  BLMatrix2D m = BLMatrix2D::makeIdentity();

  // Create mask image
  BLImage maskImage;
  maskImage.createFromData(width, height, BL_FORMAT_PRGB32, mask.Get(), rowBytes);

  // Save foreground if needed
  BLImage foreground;
  if (shadow.mDrawForeground)
  {
    foreground.create(width, height, BL_FORMAT_PRGB32);
    foreground.assignDeep(image);
  }

  // Clear the image
  ctx.setCompOp(BL_COMP_OP_CLEAR);
  ctx.fillAll();

  // Draw the mask with shadow offset
  ctx.setMatrix(m);
  ctx.setCompOp(BL_COMP_OP_SRC_OVER);
  ctx.blitImage(BLPoint(shadow.mXOffset * scale, shadow.mYOffset * scale), maskImage);

  // Apply shadow color using SrcIn
  m = BLMatrix2D::makeScaling(scale, scale);
  ctx.setMatrix(m);
  ctx.translate(-layer->Bounds().L, -layer->Bounds().T);

  IBlend blend(EBlend::Default, shadow.mOpacity);
  ctx.setCompOp(BL_COMP_OP_SRC_IN);

  if (shadow.mPattern.mType == EPatternType::Solid || shadow.mPattern.NStops() < 2)
  {
    ctx.setFillStyle(Blend2DColor(shadow.mPattern.GetStop(0).mColor, &blend));
  }
  else
  {
    ctx.setFillStyle(CreateGradient(shadow.mPattern, &blend));
  }

  ctx.fillAll();

  // Draw foreground on top if needed
  if (shadow.mDrawForeground)
  {
    m = BLMatrix2D::makeIdentity();
    ctx.setMatrix(m);
    ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    ctx.blitImage(BLPoint(0, 0), foreground);
  }

  ctx.end();
}

void IGraphicsBlend2D::DrawFastDropShadow(const IRECT& innerBounds, const IRECT& outerBounds, float xyDrop, float roundness, float blur, IBlend* pBlend)
{
  // Blend2D doesn't have a built-in box gradient like NanoVG, so we'll draw a simple shadow
  // For a more sophisticated implementation, you could use multiple rounded rectangles with decreasing opacity

  mContext.save();
  mContext.setCompOp(Blend2DCompOp(pBlend));

  // Simple shadow implementation using a blurred rounded rectangle
  BLRoundRect shadowRect(
    innerBounds.L + xyDrop,
    innerBounds.T + xyDrop,
    innerBounds.W(),
    innerBounds.H(),
    roundness,
    roundness
  );

  // Create a gradient to simulate the shadow falloff
  float cx = innerBounds.MW() + xyDrop;
  float cy = innerBounds.MH() + xyDrop;
  float radius = std::max(innerBounds.W(), innerBounds.H()) / 2.0f + blur;

  BLGradient gradient(BLRadialGradientValues(cx, cy, cx, cy, radius));
  gradient.addStop(0.0, Blend2DColor(COLOR_BLACK_DROP_SHADOW, pBlend));
  gradient.addStop(1.0, Blend2DColor(COLOR_TRANSPARENT, nullptr));

  mContext.setFillStyle(gradient);
  mContext.fillRoundRect(shadowRect);

  mContext.restore();
}

void IGraphicsBlend2D::DrawMultiLineText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* pFont = storage.Find(text.mFont);

  if (!pFont)
  {
    DrawText(text, str, bounds, pBlend);
    return;
  }

  BLFont font;
  font.createFromFace(pFont->mFontFace, text.mSize * pFont->mData->GetHeightEMRatio());

  BLFontMetrics fm = font.metrics();
  float lineHeight = fm.ascent + fm.descent + fm.lineGap;

  // Simple multi-line text implementation
  // Split text by newlines and wrap if necessary
  std::string fullText(str);
  std::vector<std::string> lines;

  size_t start = 0;
  size_t end;
  while ((end = fullText.find('\n', start)) != std::string::npos)
  {
    lines.push_back(fullText.substr(start, end - start));
    start = end + 1;
  }
  lines.push_back(fullText.substr(start));

  // Calculate total height
  float totalHeight = lines.size() * lineHeight;

  // Calculate starting Y based on vertical alignment
  float y;
  switch (text.mVAlign)
  {
    case EVAlign::Top:
      y = bounds.T + fm.ascent;
      break;
    case EVAlign::Middle:
      y = bounds.MH() - totalHeight / 2.0f + fm.ascent;
      break;
    case EVAlign::Bottom:
      y = bounds.B - totalHeight + fm.ascent;
      break;
  }

  mContext.setFillStyle(Blend2DColor(text.mFGColor, pBlend));
  mContext.setCompOp(Blend2DCompOp(pBlend));

  for (const auto& line : lines)
  {
    BLTextMetrics tm;
    BLGlyphBuffer gb;

    gb.setUtf8Text(line.c_str());
    font.shape(gb);
    font.getTextMetrics(gb, tm);

    double textWidth = tm.boundingBox.x1 - tm.boundingBox.x0;

    float x;
    switch (text.mAlign)
    {
      case EAlign::Near:
        x = bounds.L;
        break;
      case EAlign::Center:
        x = bounds.MW() - textWidth / 2.0f;
        break;
      case EAlign::Far:
        x = bounds.R - textWidth;
        break;
    }

    mContext.fillUtf8Text(BLPoint(x, y), font, line.c_str());
    y += lineHeight;
  }

  mContext.setCompOp(BL_COMP_OP_SRC_OVER);
}

const char* IGraphicsBlend2D::GetDrawingAPIStr()
{
  return "Blend2D | CPU";
}
