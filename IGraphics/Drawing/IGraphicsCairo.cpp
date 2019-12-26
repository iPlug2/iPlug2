/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cmath>

#include "png.h"

#include "IGraphicsCairo.h"
#include "ITextEntryControl.h"

using namespace iplug;
using namespace igraphics;

#pragma mark - Private Classes and Structs

class IGraphicsCairo::Bitmap : public APIBitmap
{
public:
  Bitmap(cairo_surface_t* pSurface, int scale, float drawScale);
  Bitmap(cairo_surface_t* pSurfaceType, int width, int height, int scale, float drawScale);
  virtual ~Bitmap();
};

IGraphicsCairo::Bitmap::Bitmap(cairo_surface_t* pSurface, int scale, float drawScale)
{
  cairo_surface_set_device_scale(pSurface, scale * drawScale, scale * drawScale);
  int width = cairo_image_surface_get_width(pSurface);
  int height = cairo_image_surface_get_height(pSurface);
  
  SetBitmap(pSurface, width, height, scale, drawScale);
}

IGraphicsCairo::Bitmap::Bitmap(cairo_surface_t* pSurfaceType, int width, int height, int scale, float drawScale)
{
  cairo_surface_t* pSurface;
  
  if (pSurfaceType)
    pSurface = cairo_surface_create_similar_image(pSurfaceType, CAIRO_FORMAT_ARGB32, width, height);
  else
    pSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  
  cairo_surface_set_device_scale(pSurface, scale * drawScale, scale * drawScale);
  
  SetBitmap(pSurface, width, height, scale, drawScale);
}

IGraphicsCairo::Bitmap::~Bitmap()
{
  cairo_surface_destroy(GetBitmap());
}

class IGraphicsCairo::Font
{
public:
  Font(cairo_font_face_t* font, double EMRatio) : mFont(font), mEMRatio(EMRatio) {}
  virtual ~Font() { if (mFont) cairo_font_face_destroy(mFont); }

  Font(const Font&) = delete;
  Font& operator=(const Font&) = delete;
    
  cairo_font_face_t* GetFont() const { return mFont; }
  double GetEMRatio() const { return mEMRatio; }
    
protected:
  cairo_font_face_t* mFont;
  double mEMRatio;
};

#ifdef OS_MAC
struct IGraphicsCairo::OSFont : Font
{
  OSFont(const FontDescriptor fontRef, double EMRatio) : Font(nullptr, EMRatio)
  {
    CTFontRef ctFont = CTFontCreateWithFontDescriptor(fontRef, 0.f, NULL);
    CGFontRef cgFont = CTFontCopyGraphicsFont(ctFont, NULL);
    mFont = cairo_quartz_font_face_create_for_cgfont(cgFont);
    CFRelease(ctFont);
    CGFontRelease(cgFont);
  }
};
#elif defined OS_WIN
struct IGraphicsCairo::OSFont : Font
{
  OSFont(const FontDescriptor fontRef, double EMRatio)
  : Font(cairo_win32_font_face_create_for_hfont(fontRef), EMRatio)
  {}
};

class IGraphicsCairo::PNGStream
{
public:
  PNGStream(const uint8_t* pData, int size) : mData(pData), mSize(size)
  {}

  PNGStream(const PNGStream&) = delete;
  PNGStream& operator=(const PNGStream&) = delete;
    
  static cairo_status_t Read(void *object, uint8_t* data, uint32_t length)
  {
    PNGStream* reader = reinterpret_cast<PNGStream*>(object);
    
    if ((reader->mSize -= static_cast<int>(length)) < 0)
      return CAIRO_STATUS_READ_ERROR;
      
    memcpy(data, reader->mData, length);
    reader->mData += length;
      
    return CAIRO_STATUS_SUCCESS;
  }

private:
  const uint8_t* mData;
  int mSize;
};
#endif

// Fonts
StaticStorage<IGraphicsCairo::Font> IGraphicsCairo::sFontCache;

#pragma mark - Utilites

static inline cairo_operator_t CairoBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    return CAIRO_OPERATOR_OVER;
  }
  switch (pBlend->mMethod)
  {
    case EBlend::SourceOver:      return CAIRO_OPERATOR_OVER;
    case EBlend::SourceIn:        return CAIRO_OPERATOR_IN;
    case EBlend::SourceOut:       return CAIRO_OPERATOR_OUT;
    case EBlend::SourceAtop:      return CAIRO_OPERATOR_ATOP;
    case EBlend::DestOver:        return CAIRO_OPERATOR_DEST_OVER;
    case EBlend::DestIn:          return CAIRO_OPERATOR_DEST_IN;
    case EBlend::DestOut:         return CAIRO_OPERATOR_DEST_OUT;
    case EBlend::DestAtop:        return CAIRO_OPERATOR_DEST_ATOP;
    case EBlend::Add:             return CAIRO_OPERATOR_ADD;
    case EBlend::XOR:             return CAIRO_OPERATOR_XOR;
    case EBlend::Default:         // fall through
    case EBlend::Clobber:         // fall through
    default:                      return CAIRO_OPERATOR_OVER;
  }
}

#pragma mark -

IGraphicsCairo::IGraphicsCairo(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
, mSurface(nullptr)
, mContext(nullptr)
{
  DBGMSG("IGraphics Cairo @ %i FPS\n", fps);
  
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Retain();
}

IGraphicsCairo::~IGraphicsCairo() 
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Release();
  
  // N.B. calls through to destroy context and surface
  UpdateCairoMainSurface(nullptr);
}

void IGraphicsCairo::DrawResize()
{
  SetPlatformContext(nullptr);
#ifdef OS_WIN
  HWND window = static_cast<HWND>(GetWindow());
  if (window)
  {
    HDC dc = GetDC(window);
    SetPlatformContext(dc);
    ReleaseDC(window, dc);
  }
#endif
}

APIBitmap* IGraphicsCairo::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
  cairo_surface_t* pSurface = nullptr;

#ifdef OS_WIN
  if (location == EResourceLocation::kWinBinary)
  {
    int size = 0;
    const void* pData = LoadWinResource(fileNameOrResID, "png", size, GetWinModuleHandle());
    PNGStream reader(reinterpret_cast<const uint8_t *>(pData), size);
    pSurface = cairo_image_surface_create_from_png_stream(&PNGStream::Read, &reader);
  }
  else
#endif
  if (location == EResourceLocation::kAbsolutePath)
    pSurface = cairo_image_surface_create_from_png(fileNameOrResID);

  assert(!pSurface || cairo_surface_status(pSurface) == CAIRO_STATUS_SUCCESS);

  return new Bitmap(pSurface, scale, 1.f);
}

APIBitmap* IGraphicsCairo::CreateAPIBitmap(int width, int height, int scale, double drawScale)
{
  return new Bitmap(mSurface, width, height, scale, drawScale);
}

bool IGraphicsCairo::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) /*|| (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr)*/;
}

cairo_surface_t* IGraphicsCairo::CreateCairoDataSurface(const APIBitmap* pBitmap, RawBitmapData& data, bool resize)
{
  cairo_surface_t* pSurface = nullptr;
  cairo_format_t format = CAIRO_FORMAT_ARGB32;
  int stride = cairo_format_stride_for_width(format, pBitmap->GetWidth());
  int size = stride * pBitmap->GetHeight();
  double x, y;
  
  if (resize)
  {
    data.Resize(size);
    memset(data.Get(), 0, size);
  }
  
  if (data.GetSize() >= size)
  {
    pSurface = cairo_image_surface_create_for_data(data.Get(), format, pBitmap->GetWidth(), pBitmap->GetHeight(), stride);
    cairo_surface_get_device_scale(pBitmap->GetBitmap(), &x, &y);
    cairo_surface_set_device_scale(pSurface, x, y);
  }
  
  return pSurface;
}

void IGraphicsCairo::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  cairo_surface_t *pSurface = CreateCairoDataSurface(pBitmap, data, true);
  
  if (pSurface)
  {
    cairo_t* pContext = cairo_create(pSurface);
    cairo_pattern_t *pPattern = cairo_pattern_create_for_surface(pBitmap->GetBitmap());
    cairo_set_source(pContext, pPattern);
    cairo_paint(pContext);
    cairo_pattern_destroy(pPattern);
    cairo_destroy(pContext);
  }
}

void IGraphicsCairo::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  cairo_surface_t *pSurface = CreateCairoDataSurface(pBitmap, mask, false);
  
  if (pSurface)
  {
    cairo_t* pContext = cairo_create(pBitmap->GetBitmap());

    if (!shadow.mDrawForeground)
    {
      double scale = 1.0 / (pBitmap->GetScale() * pBitmap->GetDrawScale());
      cairo_set_source_rgba(pContext, 1.0, 1.0, 1.0, 1.0);
      cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
      cairo_rectangle(pContext, 0.0, 0.0, scale * pBitmap->GetWidth(), scale * pBitmap->GetHeight());
      cairo_fill(pContext);
    }
    
    IBlend blend(EBlend::Default, shadow.mOpacity);
    cairo_translate(pContext, -layer->Bounds().L, -layer->Bounds().T);
    SetCairoSourcePattern(pContext, shadow.mPattern, &blend);
    cairo_identity_matrix(pContext);
    cairo_set_operator(pContext, shadow.mDrawForeground ? CAIRO_OPERATOR_DEST_OVER : CAIRO_OPERATOR_SOURCE);
    cairo_translate(pContext, shadow.mXOffset, shadow.mYOffset);
    cairo_mask_surface(pContext, pSurface, 0.0, 0.0);
    cairo_destroy(pContext);
  }  
}

void IGraphicsCairo::DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  cairo_save(mContext);
  cairo_rectangle(mContext, dest.L, dest.T, dest.W(), dest.H());
  cairo_clip(mContext);
  cairo_surface_t* surface = bitmap.GetAPIBitmap()->GetBitmap();
  cairo_set_source_surface(mContext, surface, dest.L - srcX, dest.T - srcY);
  cairo_set_operator(mContext, CairoBlendMode(pBlend));
  cairo_paint_with_alpha(mContext, BlendWeight(pBlend));
  cairo_restore(mContext);
}

void IGraphicsCairo::PathClear()
{
  if (mContext)
    cairo_new_path(mContext);
}

void IGraphicsCairo::PathClose()
{
  cairo_close_path(mContext);
}

void IGraphicsCairo::PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding)
{
  if (winding == EWinding::CW)
    cairo_arc(mContext, cx, cy, r, DegToRad(a1 - 90.f), DegToRad(a2 - 90.f));
  else
    cairo_arc_negative(mContext, cx, cy, r, DegToRad(a1 - 90.f), DegToRad(a2 - 90.f));
}

void IGraphicsCairo::PathMoveTo(float x, float y)
{
  cairo_move_to(mContext, x, y);
}

void IGraphicsCairo::PathLineTo(float x, float y)
{
  cairo_line_to(mContext, x, y);
}

void IGraphicsCairo::PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2)
{
  cairo_curve_to(mContext, c1x, c1y, c2x, c2y, x2, y2);
}

void IGraphicsCairo::PathQuadraticBezierTo(float cx, float cy, float x2, float y2)
{
  double x0, y0;
  cairo_get_current_point(mContext, &x0, &y0);
  cairo_curve_to(mContext,
                  2.0 / 3.0 * cx + 1.0 / 3.0 * x0,
                  2.0 / 3.0 * cy + 1.0 / 3.0 * y0,
                  2.0 / 3.0 * cx + 1.0 / 3.0 * x2,
                  2.0 / 3.0 * cy + 1.0 / 3.0 * y2,
                  x2, y2);
}

void IGraphicsCairo::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  double dashArray[8];
  
  switch (options.mCapOption)
  {
    case ELineCap::Butt:   cairo_set_line_cap(mContext, CAIRO_LINE_CAP_BUTT);     break;
    case ELineCap::Round:  cairo_set_line_cap(mContext, CAIRO_LINE_CAP_ROUND);    break;
    case ELineCap::Square: cairo_set_line_cap(mContext, CAIRO_LINE_CAP_SQUARE);   break;
  }
  
  switch (options.mJoinOption)
  {
    case ELineJoin::Miter:   cairo_set_line_join(mContext, CAIRO_LINE_JOIN_MITER);   break;
    case ELineJoin::Round:   cairo_set_line_join(mContext, CAIRO_LINE_JOIN_ROUND);   break;
    case ELineJoin::Bevel:   cairo_set_line_join(mContext, CAIRO_LINE_JOIN_BEVEL);   break;
  }
  
  cairo_set_miter_limit(mContext, options.mMiterLimit);
  
  for (int i = 0; i < options.mDash.GetCount(); i++)
    dashArray[i] = *(options.mDash.GetArray() + i);
  
  cairo_set_dash(mContext, dashArray, options.mDash.GetCount(), options.mDash.GetOffset());
  cairo_set_line_width(mContext, thickness);

  SetCairoSourcePattern(mContext, pattern, pBlend);
  if (options.mPreserve)
    cairo_stroke_preserve(mContext);
  else
    cairo_stroke(mContext);
}

void IGraphicsCairo::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) 
{
  cairo_set_fill_rule(mContext, options.mFillRule == EFillRule::EvenOdd ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
  SetCairoSourcePattern(mContext, pattern, pBlend);
  if (options.mPreserve)
    cairo_fill_preserve(mContext);
  else
    cairo_fill(mContext);
}

void IGraphicsCairo::SetCairoSourcePattern(cairo_t* context, const IPattern& pattern, const IBlend* pBlend)
{
  cairo_set_operator(context, CairoBlendMode(pBlend));
  
  switch (pattern.mType)
  {
    case EPatternType::Solid:
    {
      const IColor &color = pattern.GetStop(0).mColor;
      cairo_set_source_rgba(context, color.R / 255.0, color.G / 255.0, color.B / 255.0, (BlendWeight(pBlend) * color.A) / 255.0);
    }
    break;
      
    case EPatternType::Linear:
    case EPatternType::Radial:
    {
      cairo_pattern_t* cairoPattern;
      cairo_matrix_t matrix;
      const IMatrix& m = pattern.mTransform;
      
      if (pattern.mType == EPatternType::Linear)
        cairoPattern = cairo_pattern_create_linear(0.0, 0.0, 0.0, 1.0);
      else
        cairoPattern = cairo_pattern_create_radial(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
      
      switch (pattern.mExtend)
      {
        case EPatternExtend::None:      cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_NONE);      break;
        case EPatternExtend::Pad:       cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_PAD);       break;
        case EPatternExtend::Reflect:   cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REFLECT);   break;
        case EPatternExtend::Repeat:    cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REPEAT);    break;
      }
      
      for (int i = 0; i < pattern.NStops(); i++)
      {
        const IColorStop& stop = pattern.GetStop(i);
        cairo_pattern_add_color_stop_rgba(cairoPattern, stop.mOffset, stop.mColor.R / 255.0, stop.mColor.G / 255.0, stop.mColor.B / 255.0, (BlendWeight(pBlend) * stop.mColor.A) / 255.0);
      }
      
      cairo_matrix_init(&matrix, m.mXX, m.mYX, m.mXY, m.mYY, m.mTX, m.mTY);
      cairo_pattern_set_matrix(cairoPattern, &matrix);
      cairo_set_source(context, cairoPattern);
      cairo_pattern_destroy(cairoPattern);
    }
    break;
  }
}

IColor IGraphicsCairo::GetPoint(int x, int y)
{
  // Convert suface to cairo image surface of one pixel (avoid copying the whole surface)
    
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t* pOutContext = cairo_create(pOutSurface);
  cairo_set_source_surface(pOutContext, mSurface, -x, -y);
  cairo_paint(pOutContext);
  cairo_surface_flush(pOutSurface);

  uint8_t* pData = cairo_image_surface_get_data(pOutSurface);
  uint32_t px = *((uint32_t*)(pData));
  
  cairo_surface_destroy(pOutSurface);
  cairo_destroy(pOutContext);
    
  int A = (px >> 0) & 0xFF;
  int R = (px >> 8) & 0xFF;
  int G = (px >> 16) & 0xFF;
  int B = (px >> 24) & 0xFF;
    
  return IColor(A, R, G, B);
}

void IGraphicsCairo::PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y, cairo_glyph_t*& pGlyphs, int& numGlyphs) const
{
  cairo_text_extents_t textExtents;
  cairo_font_extents_t fontExtents;
  cairo_t* context;
  
  if (!mSurface && !mContext)
  {
    // Create a temporary context in case there is a need to measure text before the real context is created
    cairo_surface_t* pSurface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, nullptr);
    context = cairo_create(pSurface);
    cairo_surface_destroy(pSurface);
  }
  else
    context = mContext;
  
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* pCachedFont = storage.Find(text.mFont);
    
  assert(pCachedFont && "No font found - did you forget to load it?");
    
  // Get the correct font face
  
  cairo_set_font_face(context, pCachedFont->GetFont());
  cairo_set_font_size(context, text.mSize * pCachedFont->GetEMRatio());
  cairo_font_extents(context, &fontExtents);

  // Draw / measure
    
  pGlyphs = nullptr;
  numGlyphs = 0;
  cairo_scaled_font_t* pFont = cairo_get_scaled_font(context);
  cairo_scaled_font_text_to_glyphs(pFont, 0, 0, str, -1, &pGlyphs, &numGlyphs, nullptr, nullptr, nullptr);
  cairo_glyph_extents(context, pGlyphs, numGlyphs, &textExtents);
  
  const double textWidth = textExtents.width + textExtents.x_bearing;
  const double textHeight = fontExtents.height;
  const double ascender = fontExtents.ascent;
  const double descender = fontExtents.descent;
    
  switch (text.mAlign)
  {
    case EAlign::Near:     x = r.L;                          break;
    case EAlign::Center:   x = r.MW() - (textWidth / 2.0);   break;
    case EAlign::Far:      x = r.R - textWidth;              break;
  }
  
  switch (text.mVAlign)
  {
    case EVAlign::Top:      y = r.T + ascender;                            break;
    case EVAlign::Middle:   y = r.MH() - descender + (textHeight / 2.0);   break;
    case EVAlign::Bottom:   y = r.B - descender;                           break;
  }
  
  r = IRECT((float) x, (float) (y - ascender), (float) (x + textWidth), (float) (y + textHeight - ascender));
  
  // Destroy temporary context
  if (context != mContext)
    cairo_destroy(context);
}

void IGraphicsCairo::DoMeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  IRECT r = bounds;
  cairo_glyph_t* pGlyphs;
  int numGlyphs;
  double x, y;
  PrepareAndMeasureText(text, str, bounds, x, y, pGlyphs, numGlyphs);
  DoMeasureTextRotation(text, r, bounds);
  cairo_glyph_free(pGlyphs);
}

void IGraphicsCairo::DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  IRECT measured = bounds;
  cairo_glyph_t* pGlyphs;
  int numGlyphs;
  double x, y;
  
  const IColor& c = text.mFGColor;
  bool useNativeTransforms = true;

#ifdef OS_WIN
  IMatrix m = GetTransformMatrix();
  useNativeTransforms = !text.mAngle && !m.mXY && !m.mYX;
#endif 

  PrepareAndMeasureText(text, str, measured, x, y, pGlyphs, numGlyphs);
  PathTransformSave();
  
  if (useNativeTransforms)
  {
    DoTextRotation(text, bounds, measured);
    cairo_set_source_rgba(mContext, c.R / 255.0, c.G / 255.0, c.B / 255.0, (BlendWeight(pBlend) * c.A) / 255.0);
    cairo_translate(mContext, x, y);
    cairo_show_glyphs(mContext, pGlyphs, numGlyphs);
  }
  else
  {
    PathTransformSave();
    PathTransformReset();
    StartLayer(nullptr, measured);
    cairo_set_source_rgba(mContext, c.R / 255.0, c.G / 255.0, c.B / 255.0, (BlendWeight(pBlend) * c.A) / 255.0);
    cairo_translate(mContext, x, y);
    cairo_show_glyphs(mContext, pGlyphs, numGlyphs);
    ILayerPtr layer = EndLayer();
    PathTransformRestore();
    DoTextRotation(text, bounds, measured);
    DrawBitmap(layer->GetBitmap(), layer->Bounds(), 0, 0, pBlend);
  }
  
  PathTransformRestore();
  cairo_glyph_free(pGlyphs);
}

void IGraphicsCairo::UpdateCairoContext()
{
  if (mContext)
  {
    cairo_destroy(mContext);
    mContext = nullptr;
  }
  
  cairo_surface_t* pSurface = mLayers.empty() ? mSurface : mLayers.top()->GetAPIBitmap()->GetBitmap();

  if (pSurface)
    mContext = cairo_create(pSurface);
  
  //cairo_set_antialias(mContext, CAIRO_ANTIALIAS_FAST);
}

void IGraphicsCairo::UpdateCairoMainSurface(cairo_surface_t* pSurface)
{
  if (mSurface)
  {
    cairo_surface_destroy(mSurface);
    mSurface = nullptr;
  }
  
  if (pSurface)
    mSurface = pSurface;
  
  UpdateCairoContext();
}

void IGraphicsCairo::SetPlatformContext(void* pContext)
{
  if (!pContext)
  {
    UpdateCairoMainSurface(nullptr);
  }
  else if(!mSurface)
  {
#ifdef OS_MAC
    mSurface = cairo_quartz_surface_create_for_cg_context(CGContextRef(pContext), WindowWidth(), WindowHeight());
    cairo_surface_set_device_scale(mSurface, GetDrawScale(), GetDrawScale());
#elif defined OS_WIN
    mSurface = cairo_win32_surface_create_with_ddb((HDC) pContext, CAIRO_FORMAT_ARGB32, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
    cairo_surface_set_device_scale(mSurface, GetBackingPixelScale(), GetBackingPixelScale());
#else
  #error NOT IMPLEMENTED
#endif
    
    UpdateCairoContext();

    if (mContext)
    {
      cairo_set_source_rgba(mContext, 1.0, 1.0, 1.0, 1.0);
      cairo_rectangle(mContext, 0, 0, Width(), Height());
      cairo_fill(mContext);
    }
  }

  IGraphics::SetPlatformContext(pContext);
}

void IGraphicsCairo::EndFrame()
{
#ifdef OS_MAC
#elif defined OS_WIN
  cairo_surface_flush(mSurface);
  PAINTSTRUCT ps;
  HWND hWnd = (HWND) GetWindow();
  HDC dc = BeginPaint(hWnd, &ps);
  HDC cdc = cairo_win32_surface_get_dc(mSurface);
  BitBlt(dc, 0, 0, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale(), cdc, 0, 0, SRCCOPY);
  EndPaint(hWnd, &ps);
#else
#error NOT IMPLEMENTED
#endif
}

bool IGraphicsCairo::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  
  if (storage.Find(fontID))
    return true;

  IFontDataPtr data = font->GetFontData();
  
  if (!data->IsValid())
    return false;
    
  std::unique_ptr<OSFont> cairoFont(new OSFont(font->GetDescriptor(), data->GetHeightEMRatio()));

  if (cairo_font_face_status(cairoFont->GetFont()) == CAIRO_STATUS_SUCCESS)
  {
    storage.Add(cairoFont.release(), fontID);
    return true;
  }
    
  return false;
}

void IGraphicsCairo::PathTransformSetMatrix(const IMatrix& m)
{
  double xTranslate = 0.0;
  double yTranslate = 0.0;
  
  if (!mContext)
    return;
    
  if (!mLayers.empty())
  {
    IRECT bounds = mLayers.top()->Bounds();
 
    xTranslate = -bounds.L;
    yTranslate = -bounds.T;
  }
  
  cairo_matrix_t matrix1, matrix2;
  cairo_matrix_init_translate(&matrix1, xTranslate, yTranslate);
  cairo_matrix_init(&matrix2, m.mXX, m.mYX, m.mXY, m.mYY, m.mTX, m.mTY);
  cairo_matrix_multiply(&matrix1, &matrix2, &matrix1);
    
  cairo_set_matrix(mContext, &matrix1);
}

void IGraphicsCairo::SetClipRegion(const IRECT& r) 
{
  if (!mContext)
    return;
    
  cairo_reset_clip(mContext);
  if (!r.Empty())
  {
    cairo_new_path(mContext);
    cairo_rectangle(mContext, r.L, r.T, r.W(), r.H());
    cairo_clip(mContext);
  }
}
