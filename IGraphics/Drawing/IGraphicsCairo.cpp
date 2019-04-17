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

struct CairoFont
{
  CairoFont(cairo_font_face_t* font) : mFont(font) {}
  virtual ~CairoFont() { if (mFont) cairo_font_face_destroy(mFont); }
  
  cairo_font_face_t* mFont;
};

#ifdef OS_MAC
struct CairoPlatformFont : CairoFont
{
  CairoPlatformFont(const void* fontRef) : CairoFont(nullptr)
  {
    CTFontRef ctFont = CTFontCreateWithFontDescriptor((CTFontDescriptorRef) fontRef, 0.f, NULL);
    CGFontRef cgFont = CTFontCopyGraphicsFont(ctFont, NULL);
    mFont = cairo_quartz_font_face_create_for_cgfont(cgFont);
    CFRelease(ctFont);
    CGFontRelease(cgFont);
  }
};
#elif defined OS_WIN
struct CairoPlatformFont : CairoFont
{
  CairoPlatformFont(const void* fontRef) : CairoFont(cairo_win32_font_face_create_for_hfont((HFONT) fontRef))
  {}
};

class PNGStream
{
public:
  PNGStream(const uint8_t* pData, int size) : mData(pData), mSize(size)
  {}

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

static StaticStorage<CairoFont> sFontCache;

CairoBitmap::CairoBitmap(cairo_surface_t* pSurface, int scale, float drawScale)
{
  cairo_surface_set_device_scale(pSurface, scale * drawScale, scale * drawScale);
  int width = cairo_image_surface_get_width(pSurface);
  int height = cairo_image_surface_get_height(pSurface);
  
  SetBitmap(pSurface, width, height, scale, drawScale);
}

CairoBitmap::CairoBitmap(cairo_surface_t* pSurfaceType, int width, int height, int scale, float drawScale)
{
  cairo_surface_t* pSurface;
    
  if (pSurfaceType)
    pSurface = cairo_surface_create_similar_image(pSurfaceType, CAIRO_FORMAT_ARGB32, width, height);
  else
    pSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    
  cairo_surface_set_device_scale(pSurface, scale * drawScale, scale * drawScale);
  
  SetBitmap(pSurface, width, height, scale, drawScale);
}
  
CairoBitmap::~CairoBitmap()
{
  cairo_surface_destroy(GetBitmap());
}

#pragma mark -

inline cairo_operator_t CairoBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    return CAIRO_OPERATOR_OVER;
  }
  switch (pBlend->mMethod)
  {
    case kBlendDefault:         // fall through
    case kBlendClobber:         // fall through
    case kBlendSourceOver:      return CAIRO_OPERATOR_OVER;
    case kBlendSourceIn:        return CAIRO_OPERATOR_IN;
    case kBlendSourceOut:       return CAIRO_OPERATOR_OUT;
    case kBlendSourceAtop:      return CAIRO_OPERATOR_ATOP;
    case kBlendDestOver:        return CAIRO_OPERATOR_DEST_OVER;
    case kBlendDestIn:          return CAIRO_OPERATOR_DEST_IN;
    case kBlendDestOut:         return CAIRO_OPERATOR_DEST_OUT;
    case kBlendDestAtop:        return CAIRO_OPERATOR_DEST_ATOP;
    case kBlendAdd:             return CAIRO_OPERATOR_ADD;
    case kBlendXOR:             return CAIRO_OPERATOR_XOR;
  }
}

#pragma mark -

IGraphicsCairo::IGraphicsCairo(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
, mSurface(nullptr)
, mContext(nullptr)
{
  DBGMSG("IGraphics Cairo @ %i FPS\n", fps);
  
  StaticStorage<CairoFont>::Accessor storage(sFontCache);
  storage.Retain();
}

IGraphicsCairo::~IGraphicsCairo() 
{
  StaticStorage<CairoFont>::Accessor storage(sFontCache);
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

  return new CairoBitmap(pSurface, scale, 1.f);
}

APIBitmap* IGraphicsCairo::CreateAPIBitmap(int width, int height, int scale, double drawScale)
{
  return new CairoBitmap(mSurface, width, height, scale, drawScale);
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
    
    IBlend blend(kBlendDefault, shadow.mOpacity);
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

void IGraphicsCairo::PathArc(float cx, float cy, float r, float aMin, float aMax)
{
  cairo_arc(mContext, cx, cy, r, DegToRad(aMin - 90.f), DegToRad(aMax - 90.f));
}

void IGraphicsCairo::PathMoveTo(float x, float y)
{
  cairo_move_to(mContext, x, y);
}

void IGraphicsCairo::PathLineTo(float x, float y)
{
  cairo_line_to(mContext, x, y);
}

void IGraphicsCairo::PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
  cairo_curve_to(mContext, x1, y1, x2, y2, x3, y3);
}

void IGraphicsCairo::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  double dashArray[8];
  
  // First set options
  
  switch (options.mCapOption)
  {
    case kCapButt:   cairo_set_line_cap(mContext, CAIRO_LINE_CAP_BUTT);     break;
    case kCapRound:  cairo_set_line_cap(mContext, CAIRO_LINE_CAP_ROUND);    break;
    case kCapSquare: cairo_set_line_cap(mContext, CAIRO_LINE_CAP_SQUARE);   break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter:   cairo_set_line_join(mContext, CAIRO_LINE_JOIN_MITER);   break;
    case kJoinRound:   cairo_set_line_join(mContext, CAIRO_LINE_JOIN_ROUND);   break;
    case kJoinBevel:   cairo_set_line_join(mContext, CAIRO_LINE_JOIN_BEVEL);   break;
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
  cairo_set_fill_rule(mContext, options.mFillRule == kFillEvenOdd ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
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
    case kSolidPattern:
    {
      const IColor &color = pattern.GetStop(0).mColor;
      cairo_set_source_rgba(context, color.R / 255.0, color.G / 255.0, color.B / 255.0, (BlendWeight(pBlend) * color.A) / 255.0);
    }
    break;
      
    case kLinearPattern:
    case kRadialPattern:
    {
      cairo_pattern_t* cairoPattern;
      cairo_matrix_t matrix;
      const IMatrix& m = pattern.mTransform;
      
      if (pattern.mType == kLinearPattern)
        cairoPattern = cairo_pattern_create_linear(0.0, 0.0, 0.0, 1.0);
      else
        cairoPattern = cairo_pattern_create_radial(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
      
      switch (pattern.mExtend)
      {
        case kExtendNone:      cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_NONE);      break;
        case kExtendPad:       cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_PAD);       break;
        case kExtendReflect:   cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REFLECT);   break;
        case kExtendRepeat:    cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REPEAT);    break;
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

bool IGraphicsCairo::DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
  double x = 0., y = 0.;
  cairo_text_extents_t textExtents;
  cairo_font_extents_t fontExtents;

  if (measure && !mSurface && !mContext)
  {
    // TODO - make this nicer
      
    // Create a temporary context in case there is a need to measure text before the real context is created
      
    cairo_surface_t* pSurface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, nullptr);
    mContext = cairo_create(pSurface);
    cairo_surface_destroy(pSurface);
  }
  
  // Get the correct font face
  
  cairo_set_font_face(mContext, FindFont(text));
  cairo_set_font_size(mContext, text.mSize);
  cairo_font_extents(mContext, &fontExtents);

  // Set the size *again* to match the height we want (so that the ascent + descent is text.mSize
   
  double newSize = text.mSize * text.mSize / (fontExtents.ascent + fontExtents.descent);
  cairo_set_font_size(mContext, newSize);
  cairo_font_extents(mContext, &fontExtents);

  // Draw / measure
    
  cairo_scaled_font_t* pFont = cairo_get_scaled_font(mContext);
  cairo_glyph_t *pGlyphs = nullptr;
  int numGlyphs = 0;
  cairo_scaled_font_text_to_glyphs(pFont, 0, 0, str, -1, &pGlyphs, &numGlyphs, nullptr, nullptr, nullptr);
  cairo_glyph_extents(mContext, pGlyphs, numGlyphs, &textExtents);
  
  if (measure)
  {
    bounds = IRECT(0, 0, textExtents.width, textExtents.height);
    if (!mSurface)
        UpdateCairoContext();
    cairo_glyph_free(pGlyphs);
    return true;
  }

  switch (text.mAlign)
  {
    case IText::kAlignNear:     x = bounds.L;                                                                       break;
    case IText::kAlignFar:      x = bounds.R - textExtents.width - textExtents.x_bearing;                           break;
    case IText::kAlignCenter:   x = bounds.L + ((bounds.W() - textExtents.width - textExtents.x_bearing) / 2.0);    break;
    default: break;
  }
  
  switch (text.mVAlign)
  {
    case IText::kVAlignTop:      y = bounds.T + fontExtents.ascent;                                 break;
    case IText::kVAlignMiddle:   y = bounds.MH() - fontExtents.descent + fontExtents.height/2.;     break;
    case IText::kVAlignBottom:   y = bounds.B - fontExtents.descent;                                break;
    default: break;
  }
  
  bool textEntry = GetTextEntryControl() && GetTextEntryControl()->GetRECT() == bounds;
  IColor color = textEntry ? text.mTextEntryFGColor : text.mFGColor;

  cairo_save(mContext);
  cairo_set_source_rgba(mContext, color.R / 255.0, color.G / 255.0, color.B / 255.0, (BlendWeight(pBlend) * color.A) / 255.0);
  cairo_translate(mContext, x, y);
  cairo_show_glyphs(mContext, pGlyphs, numGlyphs);
  cairo_restore(mContext);
  cairo_glyph_free(pGlyphs);

  return true;
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
  BitBlt(dc, 0, 0, WindowWidth(), WindowHeight(), cdc, 0, 0, SRCCOPY);
  EndPaint(hWnd, &ps);
#else
#error NOT IMPLEMENTED
#endif
}

bool IGraphicsCairo::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<CairoFont>::Accessor storage(sFontCache);
  
  if (storage.Find(fontID))
    return true;

  std::unique_ptr<CairoPlatformFont> cairoFont(new CairoPlatformFont(font->GetDescriptor()));

  if (cairo_font_face_status(cairoFont->mFont) == CAIRO_STATUS_SUCCESS)
  {
    storage.Add(cairoFont.release(), fontID);
    return true;
  }
    
  return false;
}

cairo_font_face_t* IGraphicsCairo::FindFont(const IText& text)
{
  StaticStorage<CairoFont>::Accessor storage(sFontCache);
  CairoFont* pFont = storage.Find(text.mFont);
  
  if (pFont)
    return pFont->mFont;
  
  assert(0 && "No font found - did you forget to load it?");

  return nullptr;
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
