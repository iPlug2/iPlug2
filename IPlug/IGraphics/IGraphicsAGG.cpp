#include <cmath>

#include "IGraphicsAGG.h"
#include "IGraphicsNanoSVG.h"

static StaticStorage<agg::font> s_fontCache;

#pragma mark -

IGraphicsAGG::IGraphicsAGG(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
, mFontEngine()
, mFontManager(mFontEngine)
, mFontCurves(mFontManager.path_adaptor())
, mFontContour(mFontCurves)
{
}

IGraphicsAGG::~IGraphicsAGG()
{
}

void IGraphicsAGG::SetDisplayScale(int scale)
{
  mPixelMap.create(Width() * scale, Height() * scale);
  mRenBuf.attach(mPixelMap.buf(), mPixelMap.width(), mPixelMap.height(), mPixelMap.row_bytes());
  mPixf = PixfmtType(mRenBuf);
  mRenBase = RenbaseType(mPixf);
  mRenBase.clear(agg::rgba(0, 0, 0, 0));

  IGraphics::SetDisplayScale(scale);
}

//IFontData IGraphicsAGG::LoadFont(const char* name, const int size)
//{
//  WDL_String cacheName(name);
//  char buf [6] = {0};
//  sprintf(buf, "-%dpt", size);
//  cacheName.Append(buf);
//
//  agg::font* font_buf = s_fontCache.Find(cacheName.Get());
//  if (!font_buf)
//  {
//    font_buf = (agg::font*) OSLoadFont(name, size);
//#ifndef NDEBUG
//    bool fontResourceFound = font_buf;
//#endif
//    assert(fontResourceFound); // Protect against typos in resource.h and .rc files.
//
//    s_fontCache.Add(font_buf, cacheName.Get());
//  }
//  return IFontData(font_buf);
//}

void IGraphicsAGG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  IRECT rect = dest;
  rect.Scale(GetDisplayScale());
  
  srcX *= GetDisplayScale();
  srcY *= GetDisplayScale();

  agg::pixel_map* pPixelMap = (agg::pixel_map*) bitmap.GetRawBitmap();
  agg::rendering_buffer buf(pPixelMap->buf(), pPixelMap->width(), pPixelMap->height(), pPixelMap->row_bytes());
  
  mPixf.comp_op(AGGBlendMode(pBlend));
  
  agg::rect_i r(srcX, srcY, srcX + rect.W(), srcY + rect.H());
  mRenBase.blend_from(PixfmtType(buf), &r, -srcX + rect.L, -srcY + rect.T, AGGCover(pBlend));
}

void IGraphicsAGG::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  //TODO: blend?
  
  destCtrX *= GetDisplayScale();
  destCtrY *= GetDisplayScale();

  agg::pixel_map* pPixelMap = (agg::pixel_map*) bitmap.GetRawBitmap();
  agg::rendering_buffer buf(pPixelMap->buf(), pPixelMap->width(), pPixelMap->height(), pPixelMap->row_bytes());
  
  PixfmtType imgPixf(buf);
  
  const double width = bitmap.W() * GetDisplayScale();
  const double height = bitmap.H() * GetDisplayScale();

  agg::trans_affine srcMatrix;
  srcMatrix *= agg::trans_affine_translation(-(width / 2), -(height / 2));
  srcMatrix *= agg::trans_affine_rotation(DegToRad(angle));
  srcMatrix *= agg::trans_affine_translation(destCtrX, destCtrY);
  
  agg::trans_affine imgMtx = srcMatrix;
  imgMtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;
  
  InterpolatorType interpolator(imgMtx);

  imgSourceType imgSrc(imgPixf, agg::rgba_pre(0, 0, 0, 0));

  spanGenType sg(imgPixf, agg::rgba_pre(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, width, height, 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, srcMatrix);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, mRenBase, sa, sg);
}

void IGraphicsAGG::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend)
{
  x *= GetDisplayScale();
  y *= GetDisplayScale();

  agg::pixel_map* pm_base = (agg::pixel_map*) base.GetRawBitmap();
  agg::pixel_map* pm_mask = (agg::pixel_map*) mask.GetRawBitmap();
  agg::pixel_map* pm_top = (agg::pixel_map*) top.GetRawBitmap();
  
  agg::rendering_buffer rbuf_base(pm_base->buf(), pm_base->width(), pm_base->height(), pm_base->row_bytes());
  agg::rendering_buffer rbuf_mask(pm_mask->buf(), pm_mask->width(), pm_mask->height(), pm_mask->row_bytes());
  agg::rendering_buffer rbuf_top(pm_top->buf(), pm_top->width(), pm_top->height(), pm_top->row_bytes());

  PixfmtType img_base(rbuf_base);
  PixfmtType img_mask(rbuf_mask);
  PixfmtType img_top(rbuf_top);

  RenbaseType ren_base(img_base);
  
  ren_base.clear(agg::rgba8(255, 255, 255, 0));
  
  ren_base.blend_from(img_mask, 0, 0, agg::cover_mask);
  ren_base.copy_from(img_top);
  
  const double width = base.W() * GetDisplayScale();
  const double height = base.H() * GetDisplayScale();
  
  agg::trans_affine srcMatrix;
  srcMatrix *= agg::trans_affine_translation(-(width / 2), -(height / 2));
  srcMatrix *= agg::trans_affine_rotation(angle);
  srcMatrix *= agg::trans_affine_translation(x + (width / 2), y + (height / 2));
  
  agg::trans_affine imgMtx = srcMatrix;
  imgMtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;

  InterpolatorType interpolator(imgMtx);
  
  imgSourceType imgSrc(img_base, agg::rgba_pre(0, 0, 0, 0));
  
  spanGenType sg(img_base, agg::rgba_pre(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, width, height, 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, srcMatrix);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, mRenBase, sa, sg);
}

void IGraphicsAGG::DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  // TODO:
}

void IGraphicsAGG::PathArc(float cx, float cy, float r, float aMin, float aMax)
{
  const float s = GetDisplayScale();
  agg::arc arc(cx * s, cy * s, r * s, r * s, DegToRad(aMin), DegToRad(aMax));
  
  mPath.join_path(arc);
}

void IGraphicsAGG::PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
  const float s = GetDisplayScale();
  
  mPath.curve4(x1 * s, y1 * s, x2 * s, y2 * s, x3 * s, y3 * s);
}

void IGraphicsAGG::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  double dashArray[8];
  
  for (int i = 0; i < options.mDash.GetCount(); i++)
    dashArray[i] = *(options.mDash.GetArray() + i);
  
  // FIX - dsahing!
  
  //cairo_set_dash(mContext, dashArray, options.mDash.GetCount(), options.mDash.GetOffset());
  
  agg::conv_stroke<agg::path_storage> strokes(mPath);
 
  // Set stroke options
  
  strokes.width(thickness * GetDisplayScale());
  
  switch (options.mCapOption)
  {
    case kCapButt:   strokes.line_cap(agg::butt_cap);     break;
    case kCapRound:  strokes.line_cap(agg::round_cap);    break;
    case kCapSquare: strokes.line_cap(agg::square_cap);   break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter:   strokes.line_join(agg::miter_join);   break;
    case kJoinRound:   strokes.line_join(agg::round_join);   break;
    case kJoinBevel:   strokes.line_join(agg::bevel_join);   break;
  }
  
  // FIX - scale miter limit?
  
  strokes.miter_limit(options.mMiterLimit);
  
  Rasterize(pattern, strokes, pBlend);
  if (!options.mPreserve)
    PathClear();
}

void IGraphicsAGG::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  Rasterize(pattern, mPath, pBlend, options.mFillRule);
  if (!options.mPreserve)
    PathClear();
}

IColor IGraphicsAGG::GetPoint(int x, int y)
{
  agg::rgba8 point = mRenBase.pixel(x, y);
  IColor color(point.a, point.r, point.g, point.b);
  return color;
}

/*
IBitmap IGraphicsAGG::CropBitmap(const IBitmap& srcbitmap, const IRECT& rect, const char* cacheName, int scale)
{
  agg::pixel_map* pPixelMap = (agg::pixel_map*) srcbitmap.mAPIBitmap->GetBitmap();
  agg::pixel_map* pCopy = (agg::pixel_map*) CreateAPIBitmap(rect.W(), rect.H());
  
  agg::rendering_buffer src;
  agg::rendering_buffer dest;
  
  src.attach(pPixelMap->buf(), pPixelMap->width(), pPixelMap->height(), pPixelMap->row_bytes());
  dest.attach(pCopy->buf(), pCopy->width(), pCopy->height(), pCopy->row_bytes());
  
  PixfmtType imgPixfSrc(src);
  PixfmtType imgPixfDest(dest);
  
  RenbaseType renbase(imgPixfDest);

  renbase.clear(agg::rgba(0, 0, 0, 0));
  
  agg::rect_i src_r(rect.L, rect.T, rect.R, rect.B);
  
  renbase.copy_from(imgPixfSrc, &src_r, -rect.L, -rect.T);
  
  AGGBitmap *pAPIBitmap = new AGGBitmap(pCopy, scale);
  
  return IBitmap(new AGGBitmap(pCopy, scale));  //TODO: surface will not be destroyed, unless this is retained
}
*/
//IBitmap IGraphicsAGG::CreateIBitmap(const char* cacheName, int w, int h)
//{
//  agg::pixel_map* pPixelMap = (agg::pixel_map*) CreateAPIBitmap(w, h);
//
//  s_bitmapCache.Add(pPixelMap, cacheName, mScale);
//  
//  IBitmap bitmap(pPixelMap, pPixelMap->width(), pPixelMap->height());
//  
//  return bitmap;
//}

agg::pixel_map* IGraphicsAGG::CreateAPIBitmap(int w, int h)
{
#ifdef OS_MAC
  agg::pixel_map_mac* pPixelMap = new agg::pixel_map_mac();
#else
  //TODO: win
#endif
  
  pPixelMap->create(w, h, 0);

  return pPixelMap;
}

APIBitmap* IGraphicsAGG::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  const char *path = resourcePath.Get();
#ifdef OS_MAC
  if (CSTR_NOT_EMPTY(path))
  {
    const char* ext = path+strlen(path)-1;
    while (ext >= path && *ext != '.') --ext;
    ++ext;
    
    bool ispng = !stricmp(ext, "png");
#ifndef IPLUG_JPEG_SUPPORT
    if (!ispng) return 0;
#else
    bool isjpg = !stricmp(ext, "jpg");
    if (!isjpg && !ispng) return 0;
#endif
    
    agg::pixel_map_mac* pPixelMap = new agg::pixel_map_mac();
    if (pPixelMap->load_img(path, ispng ? agg::pixel_map::format_png : agg::pixel_map::format_jpg))
      return new AGGBitmap(pPixelMap, scale);
    else
      delete pPixelMap;
  }
  
#else // OS_WIN
  //TODO: win
#endif
  
  return new APIBitmap();
}

APIBitmap* IGraphicsAGG::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  int destW = (pBitmap->GetWidth() / pBitmap->GetScale()) * scale;
  int destH = (pBitmap->GetHeight() / pBitmap->GetScale()) * scale;
    
  agg::pixel_map* pSourcePixelMap = (agg::pixel_map*) pBitmap->GetBitmap();
  agg::pixel_map* pCopy = (agg::pixel_map*) CreateAPIBitmap(destW, destH);
  
  agg::rendering_buffer src;
  agg::rendering_buffer dest;
  
  src.attach(pSourcePixelMap->buf(), pSourcePixelMap->width(), pSourcePixelMap->height(), pSourcePixelMap->row_bytes());
  dest.attach(pCopy->buf(), pCopy->width(), pCopy->height(), pCopy->row_bytes());
  
  PixfmtType imgPixfSrc(src);
  PixfmtType imgPixfDest(dest);
  
  RenbaseType renbase(imgPixfDest);
  
  renbase.clear(agg::rgba(0, 0, 0, 0));
  
  double ratioW = (double) destW / pSourcePixelMap->width();
  double ratioH = (double) destH / pSourcePixelMap->height();
  
  agg::trans_affine srcMatrix;
  srcMatrix *= agg::trans_affine_scaling(ratioW, ratioH);
  
  agg::trans_affine imgMtx = srcMatrix;
  imgMtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;
  
  InterpolatorType interpolator(imgMtx);
  
  imgSourceType imgSrc(imgPixfSrc, agg::rgba(0, 0, 0, 0));
  
  spanGenType sg(imgPixfSrc, agg::rgba(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, pSourcePixelMap->width(), pSourcePixelMap->height(), 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, srcMatrix);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, renbase, sa, sg);
  
  return new AGGBitmap(pCopy, scale);
}

void IGraphicsAGG::RenderDrawBitmap()
{
#ifdef OS_MAC
  mPixelMap.draw((CGContext*) GetPlatformContext(), GetDisplayScale());
#else // OS_WIN
  //TODO: win
#endif
}

void IGraphicsAGG::CalculateTextLines(WDL_TypedBuf<LineInfo>* pLines, const IRECT& rect, const char* str, FontManagerType& manager)
{
  LineInfo info;
  info.mStartChar = 0;
  info.mEndChar = (int) strlen(str);
  pLines->Add(info);
  
  LineInfo* pLine = pLines->Get();
  
  size_t lineStart = 0;
  size_t lineWidth = 0;
  size_t linePos = 0;
  double xCount = 0.0;
  
  const char* cstr = str;
  
  while (*cstr)
  {
    const agg::glyph_cache* pGlyph = manager.glyph(*cstr);
    
    if (pGlyph)
    {
      xCount += pGlyph->advance_x;
    }

    cstr++;
    linePos++;
    
    if (*cstr == ' ' || *cstr == 0)
    {
      pLine->mStartChar = (int) lineStart;
      pLine->mEndChar = (int)  linePos;
      pLine->mWidth = xCount;
    }
    
    if (rect.W() > 0 && xCount >= rect.W())
    {
      assert(pLine);
      
      cstr = &str[pLine->mEndChar];
      lineStart = pLine->mEndChar + 1;
      linePos = pLine->mEndChar;
      
      LineInfo info;
      pLines->Add(info);
      pLine++;
      
      assert(pLines);
      
      xCount = 0;
      lineWidth = 0;
    }
    
  }
}

bool IGraphicsAGG::DrawText(const IText& text, const char* str, IRECT& destRect, bool measure)
{
//  if (!str || str[0] == '\0')
//  {
//    return true;
//  }
//
//  IRECT rect = destRect;
//  rect.Scale(GetDisplayScale());
//
//  RendererSolid renSolid(mRenBase);
//  RendererBin renBin(mRenBase);
//
//  agg::scanline_u8 sl;
//  agg::rasterizer_scanline_aa<> ras;
//
//  agg::glyph_rendering gren = agg::glyph_ren_agg_gray8;
//  //agg::glyph_rendering gren = agg::glyph_ren_outline;
//  //agg::glyph_rendering gren = agg::glyph_ren_agg_mono;
//  //agg::glyph_rendering gren = agg::glyph_ren_native_gray8;
//  //agg::glyph_rendering gren = agg::glyph_ren_native_mono;
//
//  float weight = 0.0;
//  bool kerning = false;
//  bool hinting = false;
//
//  if (gren == agg::glyph_ren_agg_mono)
//  {
//    mFontEngine.gamma(agg::gamma_threshold(0.5));
//  }
//  else
//  {
//    mFontEngine.gamma(agg::gamma_power(1.0));
//  }
//
//  if (gren == agg::glyph_ren_outline)
//  {
//    //for outline cache set gamma for the rasterizer
//    ras.gamma(agg::gamma_power(1.0));
//  }
//
//  mFontContour.width(-weight * (text.mSize * 0.05) * GetDisplayScale());
//
//  IFontData font = LoadFont(text.mFont, text.mSize);
//  agg::font* pFontData = (agg::font *)font.mData;
//
//  if (pFontData != 0 && mFontEngine.load_font("", 0, gren, pFontData->buf(), pFontData->size()))
//  {
//    mFontEngine.hinting(hinting);
//    mFontEngine.height(text.mSize * GetDisplayScale());
//    mFontEngine.width(text.mSize * GetDisplayScale());
//    mFontEngine.flip_y(true);
//
//    double x = rect.L;
//    double y = rect.T + (text.mSize * GetDisplayScale());
//
//    WDL_TypedBuf<LineInfo> lines;
//
//    CalculateTextLines(&lines, rect, str, mFontManager);
//
//    LineInfo * pLines = lines.Get();
//
//    for (int i=0; i<lines.GetSize(); ++i, ++pLines)
//    {
//      switch (text.mAlign)
//      {
//        case IText::kAlignNear:
//          x = rect.L;
//          break;
//        case IText::kAlignCenter:
//          x = rect.L + ((rect.W() - pLines->mWidth) / 2);
//          break;
//        case IText::kAlignFar:
//          x = rect.L + (rect.W() - pLines->mWidth);
//          break;
//      }
//
//      for (size_t c=pLines->mStartChar; c<pLines->mEndChar; c++)
//      {
//        const agg::glyph_cache* pGlyph = mFontManager.glyph(str[c]);
//
//        if (pGlyph)
//        {
//          if (kerning)
//          {
//            mFontManager.add_kerning(&x, &y);
//          }
//
//          mFontManager.init_embedded_adaptors(pGlyph, x, y);
//
//          switch (pGlyph->data_type)
//          {
//            case agg::glyph_data_mono:
//
//              renBin.color(IColorToAggColor(text.mColor));
//              agg::render_scanlines(mFontManager.mono_adaptor(),
//                                    mFontManager.mono_scanline(),
//                                    renBin);
//              break;
//
//            case agg::glyph_data_gray8:
//
//              renSolid.color(IColorToAggColor(text.mColor));
//              agg::render_scanlines(mFontManager.gray8_adaptor(),
//                                    mFontManager.gray8_scanline(),
//                                    renSolid);
//              break;
//
//            case agg::glyph_data_outline:
//
//              ras.reset();
//
//              if (fabs(weight) <= 0.01)
//              {
//                //for the sake of efficiency skip the
//                //contour converter if the weight is about zero.
//                ras.add_path(mFontCurves);
//              }
//              else
//              {
//                ras.add_path(mFontContour);
//              }
//
//              renSolid.color(IColorToAggColor(text.mColor));
//              agg::render_scanlines(ras, sl, renSolid);
//
//              break;
//
//            default: break;
//          }
//
//          //increment pen position
//          x += pGlyph->advance_x;
//          y += pGlyph->advance_y;
//        }
//      }
//      y += text.mSize * GetDisplayScale();
//    }
//  }
  return false;
}

bool IGraphicsAGG::MeasureText(const IText& text, const char* str, IRECT& destRect)
{
//  if (!str || str[0] == '\0')
//  {
//    destRect.Clear();
//    return true;
//  }

//  renderer_solid ren_solid(mRenBase);
//  renderer_bin ren_bin(mRenBase);
//
//  agg::scanline_u8 sl;
//  agg::rasterizer_scanline_aa<> ras;
//
//  agg::glyph_rendering gren = agg::glyph_ren_agg_gray8;
//  //agg::glyph_rendering gren = agg::glyph_ren_outline;
//  //agg::glyph_rendering gren = agg::glyph_ren_agg_mono;
//  //agg::glyph_rendering gren = agg::glyph_ren_native_gray8;
//  //agg::glyph_rendering gren = agg::glyph_ren_native_mono;
//
//  float weight = 0.0;
//  bool hinting = false;
//
//  if (gren == agg::glyph_ren_agg_mono)
//  {
//    mFontEngine.gamma(agg::gamma_threshold(0.5));
//  }
//  else
//  {
//    mFontEngine.gamma(agg::gamma_power(1.0));
//  }
//
//  if (gren == agg::glyph_ren_outline)
//  {
//    //for outline cache set gamma for the rasterizer
//    ras.gamma(agg::gamma_power(1.0));
//  }
//
//  mFontContour.width(-weight * (text.mSize * 0.05) * GetDisplayScale());
//
//  IFontData font = LoadFont(text.mFont, text.mSize);
//  agg::font * pFontData = (agg::font *)font.mData;
//
//  if (mFontEngine.load_font("", 0, gren, pFontData->buf(), pFontData->size()))
//  {
//    mFontEngine.hinting(hinting);
//    mFontEngine.height(text.mSize * GetDisplayScale());
//    mFontEngine.width(text.mSize * GetDisplayScale());
//    mFontEngine.flip_y(true);
//
//    WDL_TypedBuf<LineInfo> lines;
//
//    CalculateTextLines(&lines, destRect, str, mFontManager);
//
//    LineInfo * pLines = lines.Get();
//
//    int max_width = 0;
//    int height = 0;
//
//    for (int i=0; i<lines.GetSize(); ++i, ++pLines)
//    {
//      if (pLines->width > max_width)
//      {
//        max_width = pLines->width;
//      }
//      height += text.mSize * GetDisplayScale();
//    }
//
//    destRect.L = 0; destRect.T = 0;
//    destRect.R = max_width; destRect.B = height;
//
//    return true;
//  }
//
  return false;
}
/*
agg::pixel_map* IGraphicsAGG::load_image(const char* filename)
{
  IBitmap bitmap = LoadBitmap(filename, 1, 1.0);
  return (agg::pixel_map*) bitmap.mData;
}

*/

void IGraphicsAGG::Draw(const IRECT& rect)
{
  mRenBase.clear(agg::rgba(1, 1, 1));
  IGraphics::Draw(rect);
}

void IGraphicsAGG::ToPixel(float& pixel)
{
  pixel = std::floor(pixel + 0.5f) + 0.5f;
}

#include "IGraphicsAGG_src.cpp"
