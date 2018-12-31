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

#if defined OS_WIN

//TODO: could replace some of this with IGraphics::LoadWinResource
class PNGStreamReader
{
public:
  PNGStreamReader(HINSTANCE hInst, const char* path)
  : mData(nullptr), mSize(0), mCount(0)
  {
    HRSRC resInfo = FindResource(hInst, path, "PNG");
    if (resInfo)
    {
      HGLOBAL res = LoadResource(hInst, resInfo);
      if (res)
      {
        mData = (uint8_t *) LockResource(res);
        mSize = SizeofResource(hInst, resInfo);
      }
    }
  }

  cairo_status_t Read(uint8_t* data, uint32_t length)
  {
    mCount += length;
    if (mCount <= mSize)
    {
      memcpy(data, mData + mCount - length, length);
      return CAIRO_STATUS_SUCCESS;
    }

    return CAIRO_STATUS_READ_ERROR;
  }

  static cairo_status_t StaticRead(void *reader, uint8_t *data, uint32_t length)
  {
    return ((PNGStreamReader*)reader)->Read(data, length);
  }
  
private:
  const uint8_t* mData;
  size_t mCount;
  size_t mSize;
};
#endif

CairoBitmap::CairoBitmap(cairo_surface_t* pSurface, int scale, float drawScale)
{
  cairo_surface_set_device_scale(pSurface, scale * drawScale, scale * drawScale);
  int width = cairo_image_surface_get_width(pSurface);
  int height = cairo_image_surface_get_height(pSurface);
  
  SetBitmap(pSurface, width, height, scale, drawScale);
}

CairoBitmap::CairoBitmap(cairo_surface_t* pSurfaceType, int width, int height, int scale, float drawScale)
{
  cairo_surface_t* pSurface = cairo_surface_create_similar_image(pSurfaceType, CAIRO_FORMAT_ARGB32, width, height);
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
    case kBlendClobber:     return CAIRO_OPERATOR_SOURCE;
    case kBlendAdd:         return CAIRO_OPERATOR_ADD;
    case kBlendColorDodge:  return CAIRO_OPERATOR_COLOR_DODGE;
    case kBlendNone:
    default:
      return CAIRO_OPERATOR_OVER; // TODO: is this correct - same as clobber?
  }
}

#pragma mark -

IGraphicsCairo::IGraphicsCairo(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
, mSurface(nullptr)
, mContext(nullptr)
{
  DBGMSG("IGraphics Cairo @ %i FPS\n", fps);
}

IGraphicsCairo::~IGraphicsCairo() 
{
#if defined IGRAPHICS_FREETYPE
  if (mFTLibrary != nullptr)
  {
    for (auto i = 0; i < mCairoFTFaces.GetSize(); i++) {
      cairo_font_face_destroy(mCairoFTFaces.Get(i));
    }
    
    FT_Done_FreeType(mFTLibrary); // will do FT_Done_Face
  }
#endif
  
  // N.B. calls through to delete context and surface
  
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
    PNGStreamReader reader((HINSTANCE) GetWinModuleHandle(), fileNameOrResID);
    pSurface = cairo_image_surface_create_from_png_stream(&PNGStreamReader::StaticRead, &reader);
  }
  else
#endif
  if (location == EResourceLocation::kAbsolutePath)
    pSurface = cairo_image_surface_create_from_png(fileNameOrResID);

  assert(!pSurface || cairo_surface_status(pSurface) == CAIRO_STATUS_SUCCESS);

  return new CairoBitmap(pSurface, scale, 1.f);
}

APIBitmap* IGraphicsCairo::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  cairo_surface_t* pInSurface = pBitmap->GetBitmap();
  
  int destW = (pBitmap->GetWidth() / pBitmap->GetScale()) * scale;
  int destH = (pBitmap->GetHeight() / pBitmap->GetScale()) * scale;
    
  // Create resources to redraw
    
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, destW, destH);
  cairo_t* pOutContext = cairo_create(pOutSurface);
    
  // Scale and paint (destroying the context / the surface is retained)
    
  cairo_scale(pOutContext, scale, scale);
  cairo_set_source_surface(pOutContext, pInSurface, 0, 0);
  cairo_paint(pOutContext);
  cairo_destroy(pOutContext);
    
  return new CairoBitmap(pOutSurface, scale, pBitmap->GetDrawScale());
}

APIBitmap* IGraphicsCairo::CreateAPIBitmap(int width, int height)
{
  const double scale = GetBackingPixelScale();
  return new CairoBitmap(mSurface, std::round(width * scale), std::round(height * scale), GetScreenScale(), GetDrawScale());
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
    
    IBlend blend(kBlendNone, shadow.mOpacity);
    cairo_translate(pContext, -layer->Bounds().L, -layer->Bounds().T);
    SetCairoSourcePattern(pContext, shadow.mPattern, &blend);
    cairo_identity_matrix(pContext);
    cairo_set_operator(pContext, shadow.mDrawForeground ? CAIRO_OPERATOR_DEST_OVER : CAIRO_OPERATOR_SOURCE);
    cairo_translate(pContext, shadow.mXOffset, shadow.mYOffset);
    cairo_mask_surface(pContext, pSurface, 0.0, 0.0);
    cairo_destroy(pContext);
  }  
}

void IGraphicsCairo::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  const double scale = GetScreenScale() / (bitmap.GetScale() * bitmap.GetDrawScale());

  cairo_save(mContext);
  cairo_rectangle(mContext, dest.L, dest.T, dest.W(), dest.H());
  cairo_clip(mContext);
  cairo_surface_t* surface = bitmap.GetAPIBitmap()->GetBitmap();
  cairo_set_source_surface(mContext, surface, dest.L - (srcX * scale), dest.T - (srcY * scale));
  cairo_set_operator(mContext, CairoBlendMode(pBlend));
  cairo_paint_with_alpha(mContext, BlendWeight(pBlend));
  cairo_restore(mContext);
}

void IGraphicsCairo::PathClear()
{
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

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)

bool IGraphicsCairo::DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
#if defined IGRAPHICS_FREETYPE
//  FT_Face ft_face;
//
//  FT_New_Face(mFTLibrary, "/Users/oli/Applications/IGraphicsTest.app/Contents/Resources/ProFontWindows.ttf", 0, &ft_face);
//
//  FT_Set_Char_Size(ft_face, FONT_SIZE * 64, FONT_SIZE * 64, 0, 0 );
//
//  /* Create hb-ft font. */
//  hb_font_t *hb_font;
//  hb_font = hb_ft_font_create (ft_face, NULL);
//
//  /* Create hb-buffer and populate. */
//  hb_buffer_t *hb_buffer;
//  hb_buffer = hb_buffer_create ();
//  hb_buffer_add_utf8 (hb_buffer, str, -1, 0, -1);
//  hb_buffer_guess_segment_properties (hb_buffer);
//
//  /* Shape it! */
//  hb_shape (hb_font, hb_buffer, NULL, 0);
//
//  /* Get glyph information and positions out of the buffer. */
//  unsigned int len = hb_buffer_get_length (hb_buffer);
//  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
//  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);
//
//  /* Draw, using cairo. */
//  double width = 2 * MARGIN;
//  double height = 2 * MARGIN;
//  for (unsigned int i = 0; i < len; i++)
//  {
//    width  += pos[i].x_advance / 64.;
//    height -= pos[i].y_advance / 64.;
//  }
//  if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
//    height += FONT_SIZE;
//  else
//    width  += FONT_SIZE;
//
//  cairo_set_source_rgba (mContext, 1., 1., 1., 1.);
//  cairo_paint (mContext);
//  cairo_set_source_rgba (mContext, 0., 0., 0., 1.);
//  cairo_translate (mContext, MARGIN, MARGIN);
//
//  /* Set up cairo font face. */
//  cairo_font_face_t *cairo_face;
//  cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);
//  cairo_set_font_face (mContext, cairo_face);
//  cairo_set_font_size (mContext, FONT_SIZE);
//
//  /* Set up baseline. */
//  if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
//  {
//    cairo_font_extents_t font_extents;
//    cairo_font_extents (mContext, &font_extents);
//    double baseline = (FONT_SIZE - font_extents.height) * .5 + font_extents.ascent;
//    cairo_translate (mContext, 0, baseline);
//  }
//  else
//  {
//    cairo_translate (mContext, FONT_SIZE * .5, 0);
//  }
//
//  cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate (len);
//  double current_x = 0;
//  double current_y = 0;
//
//  for (unsigned int i = 0; i < len; i++)
//  {
//    cairo_glyphs[i].index = info[i].codepoint;
//    cairo_glyphs[i].x = current_x + pos[i].x_offset / 64.;
//    cairo_glyphs[i].y = -(current_y + pos[i].y_offset / 64.);
//    current_x += pos[i].x_advance / 64.;
//    current_y += pos[i].y_advance / 64.;
//  }
//  cairo_show_glyphs (mContext, cairo_glyphs, len);
//  cairo_glyph_free (cairo_glyphs);
#else // TOY text
  IColor fgColor;
  if (GetTextEntryControl() && GetTextEntryControl()->GetRECT() == bounds)
    fgColor = text.mTextEntryFGColor;
  else
    fgColor = text.mFGColor;

  cairo_set_source_rgba(mContext, fgColor.R / 255.0, fgColor.G / 255.0, fgColor.B / 255.0, (BlendWeight(pBlend) * fgColor.A) / 255.0);
  cairo_select_font_face(mContext, text.mFont, CAIRO_FONT_SLANT_NORMAL, text.mStyle == IText::kStyleBold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(mContext, text.mSize);
//  cairo_font_options_t* font_options = cairo_font_options_create ();
//  cairo_font_options_set_antialias (font_options, CAIRO_ANTIALIAS_BEST);
//  cairo_set_font_options (mContext, font_options);
  cairo_text_extents_t textExtents;
  cairo_font_extents_t fontExtents;
  cairo_font_extents(mContext, &fontExtents);
  cairo_text_extents(mContext, str, &textExtents);
//  cairo_font_options_destroy(font_options);
  
  double x = 0., y = 0.;

  switch (text.mAlign)
  {
    case IText::EAlign::kAlignNear: x = bounds.L; break;
    case IText::EAlign::kAlignFar: x = bounds.R - textExtents.width - textExtents.x_bearing; break;
    case IText::EAlign::kAlignCenter: x = bounds.L + ((bounds.W() - textExtents.width - textExtents.x_bearing) / 2.0); break;
    default: break;
  }
  
  switch (text.mVAlign)
  {
    case IText::EVAlign::kVAlignTop: y = bounds.T + fontExtents.ascent; break;
    case IText::EVAlign::kVAlignMiddle: y = bounds.MH() + (fontExtents.ascent/2.); break;
    case IText::EVAlign::kVAlignBottom: y = bounds.B - fontExtents.descent; break;
    default: break;
  }
  
  if (measure)
  {
    bounds = IRECT(0, 0, textExtents.width, fontExtents.height);
    return true;
  }

  cairo_move_to(mContext, x, y);
  cairo_show_text(mContext, str);
#endif
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

bool IGraphicsCairo::LoadFont(const char* name)
{
#ifdef IGRAPHICS_FREETYPE
  if(!mFTLibrary)
    FT_Init_FreeType(&mFTLibrary);

  WDL_String fontNameWithoutExt(name, (int) strlen(name));
  fontNameWithoutExt.remove_fileext();
  WDL_String fullPath;
  OSFindResource(name, "ttf", fullPath);

  FT_Face ftFace;
  FT_Error ftError;
  
  if (fullPath.GetLength())
  {
    ftError = FT_New_Face(mFTLibrary, fullPath.Get(), 0 /* TODO: some font files can contain multiple faces, but we don't do this*/, &ftFace);
    //TODO: error check

    mFTFaces.Add(ftFace);

    ftError = FT_Set_Char_Size(ftFace, FONT_SIZE * 64, FONT_SIZE * 64, 0, 0 ); // 72 DPI
    //TODO: error check
    cairo_font_face_t* pCairoFace = cairo_ft_font_face_create_for_ft_face(ftFace, 0);
    mCairoFTFaces.Add(pCairoFace);

    return true;
  }
#endif

  return false;
}

void IGraphicsCairo::PathTransformSetMatrix(const IMatrix& m)
{
  double xTranslate = 0.0;
  double yTranslate = 0.0;
  
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
  cairo_reset_clip(mContext);
  if (!r.Empty())
  {
    cairo_new_path(mContext);
    cairo_rectangle(mContext, r.L, r.T, r.W(), r.H());
    cairo_clip(mContext);
  }
}
