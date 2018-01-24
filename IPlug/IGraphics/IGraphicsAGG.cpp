#include <cmath>

#include "IGraphicsAGG.h"

static StaticStorage<agg::pixel_map> s_bitmapCache;
static StaticStorage<agg::font> s_fontCache;

#pragma mark -

IGraphicsAGG::IGraphicsAGG(IPlugBaseGraphics& plug, int w, int h, int fps)
: IGraphics(plug, w, h, fps)
, mFontEngine()
, mFontManager(mFontEngine)
, mFontCurves(mFontManager.path_adaptor())
, mFontContour(mFontCurves)
{
}

IGraphicsAGG::~IGraphicsAGG()
{
}

IBitmap IGraphicsAGG::LoadBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale)
{
  double targetScale = GetDisplayScale() / scale;
  double scaleRes = scale * targetScale;
  
  agg::pixel_map* pPixelMap = s_bitmapCache.Find(name, scaleRes);
  
  if (!pPixelMap) //do we have a bitmap for this display scale
  {
    WDL_String fullPath;
    bool resourceFound = OSFindResource(name, "png", fullPath);
    assert(resourceFound); // Protect against typos in resource.h and .rc files.

    if(CSTR_NOT_EMPTY(fullPath.Get()))
    {
      pPixelMap = LoadAPIBitmap(fullPath.Get());
      resourceFound = pPixelMap != nullptr;
      assert(resourceFound); // Protect against typos in resource.h and .rc files.
      
      if (scale != GetDisplayScale()) {
        IBitmap bitmap(pPixelMap, pPixelMap->width(), pPixelMap->height(), nStates, framesAreHoriztonal, scale, name);
        return ScaleBitmap(bitmap, name, targetScale);
      }
      
      s_bitmapCache.Add(pPixelMap, name, targetScale);
    }
  }
  
  return IBitmap(pPixelMap, pPixelMap->width() / targetScale, pPixelMap->height() / targetScale, nStates, framesAreHoriztonal, scale, name);
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

  agg::pixel_map* pPixelMap = (agg::pixel_map*) bitmap.mData;
  agg::rendering_buffer buf(pPixelMap->buf(), pPixelMap->width(), pPixelMap->height(), pPixelMap->row_bytes());
  
//  mPixf.comp_op(agg::comp_op_src_over);//TODO
  
  agg::rect_i r(srcX, srcY, srcX + rect.W()-1, srcY + rect.H()); //TODO: suspicious -1 here necessary to avoid problems with DrawBitmappedText
  mRenBase.blend_from(PixfmtType(buf), &r, -srcX + rect.L, -srcY + rect.T);
}

void IGraphicsAGG::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  destCtrX *= GetDisplayScale();
  destCtrY *= GetDisplayScale();

  agg::pixel_map* pPixelMap = (agg::pixel_map*) bitmap.mData;
  agg::rendering_buffer buf(pPixelMap->buf(), pPixelMap->width(), pPixelMap->height(), pPixelMap->row_bytes());
  
  PixfmtType imgPixf(buf);
  
  const double width = bitmap.W * GetDisplayScale();
  const double height = bitmap.H * GetDisplayScale();

  agg::trans_affine srcMatrix;
  srcMatrix *= agg::trans_affine_translation(-(width / 2), -(height / 2));
  srcMatrix *= agg::trans_affine_rotation(angle);
  srcMatrix *= agg::trans_affine_translation(destCtrX, destCtrY);
  
  agg::trans_affine imgMtx = srcMatrix;
  imgMtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;
  
  interpolatorType interpolator(imgMtx);

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

  agg::pixel_map* pm_base = (agg::pixel_map*)base.mData;
  agg::pixel_map* pm_mask = (agg::pixel_map*)mask.mData;
  agg::pixel_map* pm_top = (agg::pixel_map*)top.mData;
  
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
  
  const double width = base.W * GetDisplayScale();
  const double height = base.H * GetDisplayScale();
  
  agg::trans_affine srcMatrix;
  srcMatrix *= agg::trans_affine_translation(-(width / 2), -(height / 2));
  srcMatrix *= agg::trans_affine_rotation(angle);
  srcMatrix *= agg::trans_affine_translation(x + (width / 2), y + (height / 2));
  
  agg::trans_affine imgMtx = srcMatrix;
  imgMtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;

  interpolatorType interpolator(imgMtx);
  
  imgSourceType imgSrc(img_base, agg::rgba_pre(0, 0, 0, 0));
  
  spanGenType sg(img_base, agg::rgba_pre(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, width, height, 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, srcMatrix);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, mRenBase, sa, sg);
}

void IGraphicsAGG::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  mRenBase.blend_pixel(x * s, y * s, IColorToAggColor(color), 255);
}

void IGraphicsAGG::ForcePixel(const IColor& color, int x, int y)
{
  const float s = GetDisplayScale();
  mRenBase.copy_pixel(x * s, y * s, IColorToAggColor(color));
}
 
void IGraphicsAGG::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend)
{
  //ToPixel(x1);
  //ToPixel(y1);
  //ToPixel(x2);
  //ToPixel(y2);

  agg::path_storage path;
  
  const float s = GetDisplayScale();
  path.move_to(x1 * s, y1 * s);
  path.line_to(x2 * s, y2 * s);

  Stroke(color, path);
}

void IGraphicsAGG::DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  float x[3] = { x1, x2, x3 };
  float y[3] = { y1, y2, y3 };
  DrawConvexPolygon(color, x, y, 3, pBlend);
}

void IGraphicsAGG::DrawRect(const IColor& color, const IRECT& destRect, const IBlend* pBlend)
{
  float x[4] = { destRect.L, destRect.R, destRect.R, destRect.L };
  float y[4] = { destRect.T, destRect.T, destRect.B, destRect.B };
  DrawConvexPolygon(color, x, y, 4, pBlend);
}

void IGraphicsAGG::DrawRoundRect(const IColor& color, const IRECT& destRect, float cr, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  agg::rounded_rect agg_rect(destRect.L * s, destRect.T * s, (destRect.L + destRect.W()) * s, (destRect.T + destRect.H()) * s, cr * s);
  Stroke(color, agg_rect);
}

void IGraphicsAGG::DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  agg::path_storage path;
  
  path.move_to(x[0] * s, y[0] * s);
  for (int i=1; i<npoints; ++i)
  {
    path.line_to(x[i] * s, y[i] * s);
  }
  path.close_polygon();

  Stroke(color, path);
}

void IGraphicsAGG::DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  agg::arc arc(cx * s, cy * s, r * s, r * s, DegToRad(aMin), DegToRad(aMax));
  Stroke(color, arc);
}

void IGraphicsAGG::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  agg::ellipse ellipse(cx * s, cy * s, r * s, r * s);
  Stroke(color, ellipse);
}

void IGraphicsAGG::DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  // TODO:
}

void IGraphicsAGG::FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  float x[3] = { x1, x2, x3 };
  float y[3] = { y1, y2, y3 };
  FillConvexPolygon(color, x, y, 3, pBlend);
}

void IGraphicsAGG::FillRect(const IColor& color, const IRECT& destRect, const IBlend* pBlend)
{
  float x[4] = { destRect.L, destRect.R, destRect.R, destRect.L };
  float y[4] = { destRect.T, destRect.T, destRect.B, destRect.B };
  FillConvexPolygon(color, x, y, 4, pBlend);
}

void IGraphicsAGG::FillRoundRect(const IColor& color, const IRECT& destRect,  float cr, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  agg::rounded_rect agg_rect(destRect.L * s, destRect.T * s, (destRect.L + destRect.W()) * s, (destRect.T + destRect.H()) * s, cr * s);
  Fill(color, agg_rect);
}

void IGraphicsAGG::FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend)
{
  agg::path_storage path;
  const float s = GetDisplayScale();
  agg::arc arc(cx * s, cy * s, r * s, r * s, DegToRad(aMin), DegToRad(aMax));
  path.concat_path(arc);
  path.line_to(cx * s, cy * s);
  path.close_polygon();
  Fill(color, path);
}

void IGraphicsAGG::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  agg::ellipse ellipse(cx * s, cy * s, r * s, r * s);
  Fill(color, ellipse);
}

void IGraphicsAGG::FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  const float s = GetDisplayScale();
  agg::path_storage path;
  
  path.move_to(x[0] * s, y[0] * s);
  for (int i=1; i<npoints; ++i)
  {
    path.line_to(x[i] * s, y[i] * s);
  }
  path.close_polygon();
  
  Fill(color, path);
}

IColor IGraphicsAGG::GetPoint(int x, int y)
{
  agg::rgba8 point = mRenBase.pixel(x, y);
  IColor color(point.a, point.r, point.g, point.b);
  return color;
}

IBitmap IGraphicsAGG::ScaleBitmap(const IBitmap& srcbitmap, const char* cacheName, double scale)
{
  const int destW = srcbitmap.W * scale;
  const int destH = srcbitmap.H * scale;
  
  agg::pixel_map* pCopy = ScaleAPIBitmap((agg::pixel_map*) srcbitmap.mData, destW, destH);

  s_bitmapCache.Add(pCopy, cacheName, scale);
  
  return IBitmap(pCopy, destW, destH, srcbitmap.N, srcbitmap.mFramesAreHorizontal, scale, cacheName);
}

IBitmap IGraphicsAGG::CropBitmap(const IBitmap& srcbitmap, const IRECT& rect, const char* cacheName, double scale)
{
  agg::pixel_map* pPixelMap = (agg::pixel_map*)srcbitmap.mData;
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
  
  s_bitmapCache.Add(pCopy, cacheName, scale);
  
  return IBitmap(pCopy, pCopy->width(), pCopy->height(), srcbitmap.N, srcbitmap.mFramesAreHorizontal);
}

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
#ifdef OS_OSX
  agg::pixel_map_mac* pPixelMap = new agg::pixel_map_mac();
#else
  //TODO: win
#endif
  
  pPixelMap->create(w, h, 0);

  return pPixelMap;
}

agg::pixel_map* IGraphicsAGG::LoadAPIBitmap(const char* path)
{
#ifdef OS_OSX
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
    
    agg::pixel_map_mac * pPixelMap = new agg::pixel_map_mac();
    if (pPixelMap->load_img(path, ispng ? agg::pixel_map::format_png : agg::pixel_map::format_jpg))
      return pPixelMap;
    else
      delete pPixelMap;
  }
  
#else // OS_WIN
  //TODO: win
#endif
  
  return 0;
}

agg::pixel_map* IGraphicsAGG::ScaleAPIBitmap(agg::pixel_map* pSourcePixelMap, int destW, int destH)
{
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
  
  interpolatorType interpolator(imgMtx);
  
  imgSourceType imgSrc(imgPixfSrc, agg::rgba(0, 0, 0, 0));
  
  spanGenType sg(imgPixfSrc, agg::rgba(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, pSourcePixelMap->width(), pSourcePixelMap->height(), 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, srcMatrix);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, renbase, sa, sg);
  
  return pCopy;
}

void IGraphicsAGG::RenderDrawBitmap()
{
#ifdef OS_OSX
  mPixelMap.draw((CGContext*) GetPlatformContext(), GetDisplayScale() / GetScale());
#else // OS_WIN
  //TODO: win
#endif
}

void IGraphicsAGG::CalculateTextLines(WDL_TypedBuf<LineInfo> * lines, const IRECT& rect, const char* str, FontManagerType& manager)
{
  LineInfo info;
  info.mStartChar = 0;
  info.mEndChar = (int) strlen(str);
  lines->Add(info);
  
  LineInfo* pLines = lines->Get();
  
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
      xCount +pGlyph->advance_x;
    }

    cstr++;
    linePos++;
    
    if (*cstr == ' ' || *cstr == 0)
    {
      pLines->mStartChar = (int) lineStart;
      pLines->mEndChar = (int)  linePos;
      pLines->mWidth = xCount;
    }
    
    if (rect.W() > 0 && xCount >= rect.W())
    {
      assert(pLines);
      
      cstr = &str[pLines->mEndChar];
      lineStart = pLines->mEndChar + 1;
      linePos = pLines->mEndChar;
      
      LineInfo info;
      lines->Add(info);
      pLines++;
      
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
//  agg::font * font_data = (agg::font *)font.mData;
//
//  if (font_data != 0 && mFontEngine.load_font("", 0, gren, font_data->buf(), font_data->size()))
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
//          x = rect.L + ((rect.W() - pLines->width) / 2);
//          break;
//        case IText::kAlignFar:
//          x = rect.L + (rect.W() - pLines->width);
//          break;
//      }
//
//      for (size_t c=pLines->start_char; c<pLines->end_char; c++)
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
//          switch (pGlyph->dataType)
//          {
//            case agg::glyph_data_mono:
//
//              ren_bin.color(IColorToAggColor(text.mColor));
//              agg::render_scanlines(mFontManager.mono_adaptor(),
//                                    mFontManager.mono_scanline(),
//                                    ren_bin);
//              break;
//
//            case agg::glyph_data_gray8:
//
//              ren_solid.color(IColorToAggColor(text.mColor));
//              agg::render_scanlines(mFontManager.gray8_adaptor(),
//                                    mFontManager.gray8_scanline(),
//                                    ren_solid);
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
//              ren_solid.color(IColorToAggColor(text.mColor));
//              agg::render_scanlines(ras, sl, ren_solid);
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
//  agg::font * font_data = (agg::font *)font.mData;
//
//  if (mFontEngine.load_font("", 0, gren, font_data->buf(), font_data->size()))
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

/*void IGraphicsAGG::DrawSVG(agg::svg::path_renderer& pathRenderer, const IRECT& rect)
{
  agg::trans_affine mtx;
  mtx.scale(GetDisplayScale());
  mtx.translate(rect.L, rect.T);
  
  agg::rasterizer_scanline_aa<> pf;
  agg::scanline_p8 sl;
  
  agg::rect_i cb(0, 0, Width() * GetDisplayScale(), Height() * GetDisplayScale());
  
  agg::pixfmt_gray8 pixf(mAlphaMaskRenBuf);
  mask_ren_base maskRenBase(pixf);
  
  agg::alpha_mask_gray8 mask(mAlphaMaskRenBuf);
  scanlineType msl(mask);
    
#ifdef WIN32
  agg::alpha_mask_rgba32gray img_mask_pixf;
#else
  agg::alpha_mask_argb32gray img_mask_pixf;
#endif
  
  pathRenderer.render(pf, sl, msl, mRenBase, maskRenBase, mPixf, img_mask_pixf, mtx, cb, *this);
  }


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
