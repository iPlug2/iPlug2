#include "IGraphicsAGG.h"
#include "IControl.h"
#include <cmath>
#include "Log.h"


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

IBitmap IGraphicsAGG::LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale)
{
  double targetScale = mDisplayScale / scale;
  double scaleRes = scale * targetScale;
  
  agg::pixel_map * pixel_map = s_bitmapCache.Find(name, scaleRes);
  
  if (!pixel_map) //do we have a bitmap for this display scale
  {
    WDL_String fullPath;
    OSLoadBitmap(name, fullPath);

    if(CSTR_NOT_EMPTY(fullPath.Get()))
    {
      pixel_map = LoadAPIBitmap(fullPath.Get());
  #ifndef NDEBUG
      bool imgResourceFound = pixel_map;
  #endif
      assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
      
      if (scale != mDisplayScale) {
        IBitmap bitmap(pixel_map, pixel_map->width(), pixel_map->height(), nStates, framesAreHoriztonal, scale, name);
        return ScaleIBitmap(bitmap, name, targetScale);
      }
      
      s_bitmapCache.Add(pixel_map, name, targetScale);
    }
  }
  
  return IBitmap(pixel_map, pixel_map->width() / targetScale, pixel_map->height() / targetScale, nStates, framesAreHoriztonal, scale, name);
}

void IGraphicsAGG::ReScale()
{
  // resize draw bitmap
  PrepDraw();
  
  const int cacheSize = s_bitmapCache.mDatas.GetSize();
  
  for ( int i = 0; i<cacheSize; i++)
  {
    const char* path = s_bitmapCache.mDatas.Get(i)->path.Get();

    WDL_String fullPath;
    OSLoadBitmap(path, fullPath);
    
    agg::pixel_map* pixel_map = s_bitmapCache.Find(fullPath.Get(), 1.);
    
    if (!pixel_map)
    {
      if(CSTR_NOT_EMPTY(fullPath.Get()))
      {
        pixel_map = LoadAPIBitmap(fullPath.Get());
        if (pixel_map)
        {
          s_bitmapCache.Add(pixel_map, fullPath.Get(), 1.);
        }
      }
    }
  }
  
//  printf("cache size %i\n", s_bitmapCache.mDatas.GetSize());
  
  // notify IControls
  IGraphics::ReScale();
}

IFontData IGraphicsAGG::LoadIFont(const char* name, const int size)
{
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
}

void IGraphicsAGG::PrepDraw()
{
  int w = Width() * mDisplayScale;
  int h = Height() * mDisplayScale;
  
  mPixelMap.create(w, h);
  mRenBuf.attach(mPixelMap.buf(), mPixelMap.width(), mPixelMap.height(), mPixelMap.row_bytes());
  mPixf = PixfmtType(mRenBuf);
  mRenBase = RenbaseType(mPixf);
  mRenBase.clear(agg::rgba(0, 0, 0, 0));
}

void IGraphicsAGG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend)
{
  IRECT rect = dest;
  rect.Scale(mDisplayScale);
  
  srcX *= mDisplayScale;
  srcY *= mDisplayScale;

  agg::pixel_map * pixel_map = (agg::pixel_map *) bitmap.mData;
  agg::rendering_buffer buf(pixel_map->buf(), pixel_map->width(), pixel_map->height(), pixel_map->row_bytes());
  
  mPixf.comp_op(agg::comp_op_src_over);//TODO
  
  agg::rect_i r(srcX, srcY, srcX + rect.W(), srcY + rect.H());
  mRenBase.blend_from(PixfmtType(buf), &r, -srcX + rect.L, -srcY + rect.T);
}

void IGraphicsAGG::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend)
{
  destCtrX *= mScale;
  destCtrY *= mScale;
  
  typedef agg::span_interpolator_linear<> interpolator_type;
  typedef agg::image_accessor_clip<PixfmtType> img_source_type;
  typedef agg::span_image_filter_rgba_bilinear_clip <PixfmtType, interpolator_type> span_gen_type;
  
  agg::pixel_map * pixel_map = (agg::pixel_map *)bitmap.mData;
  agg::rendering_buffer buf(pixel_map->buf(), pixel_map->width(), pixel_map->height(), pixel_map->row_bytes());
  
  PixfmtType img_pixf(buf);
  
  const double width = bitmap.W * mScale;
  const double height = bitmap.H * mScale;

  agg::trans_affine src_mtx;
  src_mtx *= agg::trans_affine_translation(-(width / 2), -(height / 2));
  src_mtx *= agg::trans_affine_rotation(angle);
  src_mtx *= agg::trans_affine_translation(destCtrX, destCtrY);
  
  agg::trans_affine img_mtx = src_mtx;
  img_mtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;
  
  interpolator_type interpolator(img_mtx);

  img_source_type img_src(img_pixf, agg::rgba_pre(0, 0, 0, 0));

  span_gen_type sg(img_pixf, agg::rgba_pre(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, width, height, 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, src_mtx);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, mRenBase, sa, sg);
}

void IGraphicsAGG::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend)
{
  x *= mScale;
  y *= mScale;
  
  typedef agg::span_interpolator_linear<> interpolator_type;
  typedef agg::image_accessor_clip<PixfmtType> img_source_type;
  typedef agg::span_image_filter_rgba_bilinear_clip <PixfmtType, interpolator_type> span_gen_type;
  
  agg::pixel_map * pm_base = (agg::pixel_map *)base.mData;
  agg::pixel_map * pm_mask = (agg::pixel_map *)mask.mData;
  agg::pixel_map * pm_top = (agg::pixel_map *)top.mData;
  
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
  
  const double width = base.W * mScale;
  const double height = base.H * mScale;
  
  agg::trans_affine src_mtx;
  src_mtx *= agg::trans_affine_translation(-(width / 2), -(height / 2));
  src_mtx *= agg::trans_affine_rotation(angle);
  src_mtx *= agg::trans_affine_translation(x + (width / 2), y + (height / 2));
  
  agg::trans_affine img_mtx = src_mtx;
  img_mtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;

  interpolator_type interpolator(img_mtx);
  
  img_source_type img_src(img_base, agg::rgba_pre(0, 0, 0, 0));
  
  span_gen_type sg(img_base, agg::rgba_pre(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, width, height, 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, src_mtx);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, mRenBase, sa, sg);
}

void IGraphicsAGG::DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa)
{
  mRenBase.blend_pixel(x * mScale, y * mScale, IColorToAggColor(color), 255);
}

void IGraphicsAGG::ForcePixel(const IColor& color, int x, int y)
{
  mRenBase.copy_pixel(x * mScale, y * mScale, IColorToAggColor(color));
}

void IGraphicsAGG::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa)
{
  ToPixel(x1);
  ToPixel(y1);
  ToPixel(x2);
  ToPixel(y2);
  
  typedef agg::conv_stroke<agg::path_storage> agg_strokes;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::path_storage path;
  agg_strokes strokes(path);
  
  path.move_to(x1, y1);
  path.line_to(x2, y2);
  
  strokes.width(1.0);
  
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.reset();
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  
  rasterizer.filling_rule(agg::fill_non_zero);
  rasterizer.add_path(strokes);
  
  agg::render_scanlines(rasterizer, scanline, renderer);
}

void IGraphicsAGG::DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend, bool aa)
{
  cx *= mScale;
  cy *= mScale;
  r *= mScale;
  
  typedef agg::conv_stroke<agg::arc> agg_strokes;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::arc arc(cx, cy, r, r, agg::deg2rad(minAngle), agg::deg2rad(maxAngle));
  agg_strokes strokes(arc);
  
  strokes.width(1.0);
  
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.reset();
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  
  rasterizer.filling_rule(agg::fill_non_zero);
  rasterizer.reset();
  rasterizer.add_path(strokes);
  agg::render_scanlines(rasterizer, scanline, renderer);
}

void IGraphicsAGG::DrawCircle(const IColor& color, float cx, float cy, float r, const IChannelBlend* pBlend, bool aa)
{
  cx *= mScale;
  cy *= mScale;
  r *= mScale;
  
  typedef agg::conv_stroke<agg::ellipse> agg_strokes;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::ellipse ellipse(cx, cy, r, r);
  agg_strokes strokes(ellipse);
  
  strokes.width(1.0);
  
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.reset();
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  
  rasterizer.filling_rule(agg::fill_non_zero);
  rasterizer.reset();
  rasterizer.add_path(strokes);
  agg::render_scanlines(rasterizer, scanline, renderer);
}

void IGraphicsAGG::DrawRoundRect(const IColor& color, const IRECT& destRect, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  IRECT rect = destRect;
  rect.Scale(mScale);
  
  typedef agg::conv_stroke<agg::rounded_rect> agg_strokes;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::rounded_rect agg_rect(rect.L - 0.5, rect.T - 0.5, rect.L - 0.5 + rect.W(), rect.T - 0.5 + rect.H(), cornerradius);
  agg_strokes strokes(agg_rect);
  
  strokes.width(1.0);
  
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.reset();
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  
  rasterizer.filling_rule(agg::fill_non_zero);
  rasterizer.reset();
  rasterizer.add_path(strokes);
  agg::render_scanlines(rasterizer, scanline, renderer);
}

void IGraphicsAGG::FillRoundRect(const IColor& color, const IRECT& destRect, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  IRECT rect = destRect;
  rect.Scale(mScale);
  
  typedef agg::conv_stroke<agg::path_storage> agg_strokes;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::rounded_rect agg_rect(rect.L, rect.T, rect.L + rect.W(), rect.T + rect.H(), cornerradius);
  
  agg::rasterizer_scanline_aa<> rasterizer;
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  
  rasterizer.add_path(agg_rect);
  agg::render_scanlines(rasterizer, scanline, renderer);
}

void IGraphicsAGG::FillIRect(const IColor& color, const IRECT& destRect, const IChannelBlend* pBlend)
{
  IRECT rect = destRect;
  rect.Scale(mDisplayScale);
  
  typedef agg::conv_stroke<agg::path_storage> agg_strokes;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::path_storage path;

  path.move_to(rect.L, rect.T);
  path.line_to(rect.R, rect.T);
  path.line_to(rect.R, rect.B);
  path.line_to(rect.L, rect.B);
  
  path.close_polygon();
  
  agg::rasterizer_scanline_aa<> rasterizer;
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  
  rasterizer.add_path(path);
  agg::render_scanlines(rasterizer, scanline, renderer);
}

void IGraphicsAGG::FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa)
{
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::ellipse ellipse(cx * mScale, cy * mScale, r * mScale, r * mScale);
  
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.reset();
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  rasterizer.reset();
  
  rasterizer.add_path(ellipse);
  agg::render_scanlines(rasterizer, scanline, renderer);
}

void IGraphicsAGG::FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
  int x[3] = { x1, x2, x3 };
  int y[3] = { y1, y2, y3 };
  FillIConvexPolygon(color, x, y, 3, pBlend);
}

void IGraphicsAGG::FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend)
{
  typedef agg::conv_stroke<agg::path_storage> agg_strokes;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_type;
  
  agg::path_storage path;
  
  path.move_to(x[0] * mScale, y[0] * mScale);
  
  for (int i=1; i<npoints; ++i)
  {
    path.line_to(x[i] * mScale, y[i] * mScale);
  }

  path.close_polygon();
  
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.reset();
  
  agg::scanline_p8 scanline;
  
  renderer_type renderer(mRenBase);
  
  renderer.color(IColorToAggColor(color));
  rasterizer.reset();
  
  rasterizer.add_path(path);
  agg::render_scanlines(rasterizer, scanline, renderer);
}

IColor IGraphicsAGG::GetPoint(int x, int y)
{
  agg::rgba8 point = mRenBase.pixel(x, y);
  IColor color(point.a, point.r, point.g, point.b);
  return color;
}

IBitmap IGraphicsAGG::ScaleIBitmap(const IBitmap& srcbitmap, const char* cacheName, double scale)
{
  const int destW = srcbitmap.W * scale;
  const int destH = srcbitmap.H * scale;
  
  agg::pixel_map* copy = ScaleAPIBitmap((agg::pixel_map*) srcbitmap.mData, destW, destH);

  s_bitmapCache.Add(copy, cacheName, scale);
  
  return IBitmap(copy, destW, destH, srcbitmap.N, srcbitmap.mFramesAreHorizontal, scale, cacheName);
}

IBitmap IGraphicsAGG::CropIBitmap(const IBitmap& srcbitmap, const IRECT& rect, const char* cacheName, double scale)
{
  agg::pixel_map* pixel_map = (agg::pixel_map *)srcbitmap.mData;
  agg::pixel_map* copy = (agg::pixel_map*) CreateAPIBitmap(rect.W(), rect.H());
  
  agg::rendering_buffer src;
  agg::rendering_buffer dest;
  
  src.attach(pixel_map->buf(), pixel_map->width(), pixel_map->height(), pixel_map->row_bytes());
  dest.attach(copy->buf(), copy->width(), copy->height(), copy->row_bytes());
  
  PixfmtType img_pixf_src(src);
  PixfmtType img_pixf_dest(dest);
  
  RenbaseType renbase(img_pixf_dest);

  renbase.clear(agg::rgba(0, 0, 0, 0));
  
  agg::rect_i src_r(rect.L, rect.T, rect.R, rect.B);
  
  renbase.copy_from(img_pixf_src, &src_r, -rect.L, -rect.T);
  
  s_bitmapCache.Add(copy, cacheName, scale);
  
  return IBitmap(copy, copy->width(), copy->height(), srcbitmap.N, srcbitmap.mFramesAreHorizontal);
}

//IBitmap IGraphicsAGG::CreateIBitmap(const char* cacheName, int w, int h)
//{
//  agg::pixel_map * pixel_map = (agg::pixel_map*) CreateAPIBitmap(w, h);
//
//  s_bitmapCache.Add(pixel_map, cacheName, mScale);
//  
//  IBitmap bitmap(pixel_map, pixel_map->width(), pixel_map->height());
//  
//  return bitmap;
//}

agg::pixel_map* IGraphicsAGG::CreateAPIBitmap(int w, int h)
{
#ifdef OS_OSX
  agg::pixel_map_mac* pixel_map = new agg::pixel_map_mac();
#else
  //TODO: win
#endif
  
  pixel_map->create(w, h, 0);

  return pixel_map;
}

agg::pixel_map* IGraphicsAGG::LoadAPIBitmap(const char* pPath)
{
#ifdef OS_OSX
  if (CSTR_NOT_EMPTY(pPath))
  {
    const char* ext = pPath+strlen(pPath)-1;
    while (ext >= pPath && *ext != '.') --ext;
    ++ext;
    
    bool ispng = !stricmp(ext, "png");
#ifndef IPLUG_JPEG_SUPPORT
    if (!ispng) return 0;
#else
    bool isjpg = !stricmp(ext, "jpg");
    if (!isjpg && !ispng) return 0;
#endif
    
    agg::pixel_map_mac * pixel_map = new agg::pixel_map_mac();
    if (pixel_map->load_img(pPath, ispng ? agg::pixel_map::format_png : agg::pixel_map::format_jpg))
      return pixel_map;
    else
      delete pixel_map;
  }
  
#else // OS_WIN
  //TODO: win
#endif
  
  return 0;
}

agg::pixel_map* IGraphicsAGG::ScaleAPIBitmap(agg::pixel_map* src_pixel_map, int destW, int destH)
{
  agg::pixel_map* copy = (agg::pixel_map*) CreateAPIBitmap(destW, destH);
  
  agg::rendering_buffer src;
  agg::rendering_buffer dest;
  
  src.attach(src_pixel_map->buf(), src_pixel_map->width(), src_pixel_map->height(), src_pixel_map->row_bytes());
  dest.attach(copy->buf(), copy->width(), copy->height(), copy->row_bytes());
  
  PixfmtType img_pixf_src(src);
  PixfmtType img_pixf_dest(dest);
  
  RenbaseType renbase(img_pixf_dest);
  
  renbase.clear(agg::rgba(0, 0, 0, 0));
  
  double ratioW = (double)destW/src_pixel_map->width();
  double ratioH = (double)destH/src_pixel_map->height();
  
  agg::trans_affine src_mtx;
  src_mtx *= agg::trans_affine_scaling(ratioW, ratioH);
  
  agg::trans_affine img_mtx = src_mtx;
  img_mtx.invert();
  
  agg::span_allocator<agg::rgba8> sa;
  
  typedef agg::span_interpolator_linear<> interpolator_type;
  interpolator_type interpolator(img_mtx);
  
  typedef agg::image_accessor_clip<PixfmtType> img_source_type;
  
  img_source_type img_src(img_pixf_src, agg::rgba(0, 0, 0, 0));
  
  typedef agg::span_image_filter_rgba_bilinear_clip <PixfmtType, interpolator_type> span_gen_type;
  
  span_gen_type sg(img_pixf_src, agg::rgba(0, 0, 0, 0), interpolator);
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  
  agg::rounded_rect rect(0, 0, src_pixel_map->width(), src_pixel_map->height(), 0);
  
  agg::conv_transform<agg::rounded_rect> tr(rect, src_mtx);
  
  ras.add_path(tr);
  agg::render_scanlines_aa(ras, sl, renbase, sa, sg);
  
  return copy;
}

void IGraphicsAGG::RenderAPIBitmap(void *pContext)
{
#ifdef OS_OSX
  mPixelMap.draw((CGContext*) pContext, mDisplayScale);
#else // OS_WIN
  //TODO: win
#endif
}

void IGraphicsAGG::CalculateTextLines(WDL_TypedBuf<LineInfo> * lines, const IRECT& rect, const char* str, FontManagerType& manager)
{
  LineInfo info;
  info.start_char = 0;
  info.end_char = (int) strlen(str);
  lines->Add(info);
  
  LineInfo * pLines = lines->Get();
  
  size_t line_start = 0;
  size_t line_width = 0;
  size_t line_pos = 0;
  double x_count = 0.0;
  
  const char* string = str;
  
  while (*string)
  {
    const agg::glyph_cache * glyph = manager.glyph(*string);
    
    if (glyph)
    {
      x_count += glyph->advance_x;
    }

    string++;
    line_pos++;
    
    if (*string == ' ' || *string == 0)
    {
      pLines->start_char = line_start;
      pLines->end_char = line_pos;
      pLines->width = x_count;
    }
    
    if (rect.W() > 0 && x_count >= rect.W())
    {
      assert(pLines);
      
      string = &str[pLines->end_char];
      line_start = pLines->end_char + 1;
      line_pos = pLines->end_char;
      
      LineInfo info;
      lines->Add(info);
      pLines++;
      
      assert(pLines);
      
      x_count = 0;
      line_width = 0;
    }
    
  }
}

bool IGraphicsAGG::DrawIText(const IText& text, const char* str, IRECT& destRect, bool measure)
{
  if (!str || str[0] == '\0')
  {
    return true;
  }
  
  IRECT rect = destRect;
  rect.Scale(mScale);
  
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_solid;
  typedef agg::renderer_scanline_bin_solid<RenbaseType> renderer_bin;
  
  renderer_solid ren_solid(mRenBase);
  renderer_bin ren_bin(mRenBase);
  
  agg::scanline_u8 sl;
  agg::rasterizer_scanline_aa<> ras;
  
  agg::glyph_rendering gren = agg::glyph_ren_agg_gray8;
  //agg::glyph_rendering gren = agg::glyph_ren_outline;
  //agg::glyph_rendering gren = agg::glyph_ren_agg_mono;
  //agg::glyph_rendering gren = agg::glyph_ren_native_gray8;
  //agg::glyph_rendering gren = agg::glyph_ren_native_mono;
  
  float weight = 0.0;
  bool kerning = false;
  bool hinting = false;
  
  if (gren == agg::glyph_ren_agg_mono)
  {
    mFontEngine.gamma(agg::gamma_threshold(0.5));
  }
  else
  {
    mFontEngine.gamma(agg::gamma_power(1.0));
  }
  
  if (gren == agg::glyph_ren_outline)
  {
    //for outline cache set gamma for the rasterizer
    ras.gamma(agg::gamma_power(1.0));
  }
  
  mFontContour.width(-weight * (text.mSize * 0.05) * mScale);
  
  IFontData font = LoadIFont(text.mFont, text.mSize);
  agg::font * font_data = (agg::font *)font.mData;
  
  if (font_data != 0 && mFontEngine.load_font("", 0, gren, font_data->buf(), font_data->size()))
  {
    mFontEngine.hinting(hinting);
    mFontEngine.height(text.mSize * mScale);
    mFontEngine.width(text.mSize * mScale);
    mFontEngine.flip_y(true);
    
    double x = rect.L;
    double y = rect.T + (text.mSize * mScale);
    
    WDL_TypedBuf<LineInfo> lines;
    
    CalculateTextLines(&lines, rect, str, mFontManager);
  
    LineInfo * pLines = lines.Get();
    
    for (int i=0; i<lines.GetSize(); ++i, ++pLines)
    {
      switch (text.mAlign)
      {
        case IText::kAlignNear:
          x = rect.L;
          break;
        case IText::kAlignCenter:
          x = rect.L + ((rect.W() - pLines->width) / 2);
          break;
        case IText::kAlignFar:
          x = rect.L + (rect.W() - pLines->width);
          break;
      }
      
      for (size_t c=pLines->start_char; c<pLines->end_char; c++)
      {
        const agg::glyph_cache * glyph = mFontManager.glyph(str[c]);
        
        if (glyph)
        {
          if (kerning)
          {
            mFontManager.add_kerning(&x, &y);
          }
          
          mFontManager.init_embedded_adaptors(glyph, x, y);
          
          switch (glyph->data_type)
          {
            case agg::glyph_data_mono:
              
              ren_bin.color(IColorToAggColor(text.mColor));
              agg::render_scanlines(mFontManager.mono_adaptor(),
                                    mFontManager.mono_scanline(),
                                    ren_bin);
              break;
              
            case agg::glyph_data_gray8:

              ren_solid.color(IColorToAggColor(text.mColor));
              agg::render_scanlines(mFontManager.gray8_adaptor(),
                                    mFontManager.gray8_scanline(),
                                    ren_solid);
              break;
              
            case agg::glyph_data_outline:
              
              ras.reset();
              
              if (fabs(weight) <= 0.01)
              {
                //for the sake of efficiency skip the
                //contour converter if the weight is about zero.
                ras.add_path(mFontCurves);
              }
              else
              {
                ras.add_path(mFontContour);
              }
              
              ren_solid.color(IColorToAggColor(text.mColor));
              agg::render_scanlines(ras, sl, ren_solid);
              
              break;
              
            default: break;
          }
          
          //increment pen position
          x += glyph->advance_x;
          y += glyph->advance_y;
        }
      }
      y += text.mSize * mScale;
    }
  }
  }

bool IGraphicsAGG::MeasureIText(const IText& text, const char* str, IRECT& destRect)
{
  if (!str || str[0] == '\0')
  {
    destRect.Clear();
    return true;
  }
  
  typedef agg::renderer_scanline_aa_solid<RenbaseType> renderer_solid;
  typedef agg::renderer_scanline_bin_solid<RenbaseType> renderer_bin;
  
  renderer_solid ren_solid(mRenBase);
  renderer_bin ren_bin(mRenBase);
  
  agg::scanline_u8 sl;
  agg::rasterizer_scanline_aa<> ras;
  
  agg::glyph_rendering gren = agg::glyph_ren_agg_gray8;
  //agg::glyph_rendering gren = agg::glyph_ren_outline;
  //agg::glyph_rendering gren = agg::glyph_ren_agg_mono;
  //agg::glyph_rendering gren = agg::glyph_ren_native_gray8;
  //agg::glyph_rendering gren = agg::glyph_ren_native_mono;
  
  float weight = 0.0;
  bool hinting = false;
  
  if (gren == agg::glyph_ren_agg_mono)
  {
    mFontEngine.gamma(agg::gamma_threshold(0.5));
  }
  else
  {
    mFontEngine.gamma(agg::gamma_power(1.0));
  }
  
  if (gren == agg::glyph_ren_outline)
  {
    //for outline cache set gamma for the rasterizer
    ras.gamma(agg::gamma_power(1.0));
  }
  
  mFontContour.width(-weight * (text.mSize * 0.05) * mScale);
  
  IFontData font = LoadIFont(text.mFont, text.mSize);
  agg::font * font_data = (agg::font *)font.mData;
  
  if (mFontEngine.load_font("", 0, gren, font_data->buf(), font_data->size()))
  {
    mFontEngine.hinting(hinting);
    mFontEngine.height(text.mSize * mScale);
    mFontEngine.width(text.mSize * mScale);
    mFontEngine.flip_y(true);
    
    WDL_TypedBuf<LineInfo> lines;
    
    CalculateTextLines(&lines, destRect, str, mFontManager);
    
    LineInfo * pLines = lines.Get();
    
    int max_width = 0;
    int height = 0;
    
    for (int i=0; i<lines.GetSize(); ++i, ++pLines)
    {
      if (pLines->width > max_width)
      {
        max_width = pLines->width;
      }
      height += text.mSize * mScale;
    }
    
    destRect.L = 0; destRect.T = 0;
    destRect.R = max_width; destRect.B = height;
    
    return true;
  }
  
  return false;
}

/*void IGraphicsAGG::DrawSVG(agg::svg::path_renderer& pathRenderer, const IRECT& rect)
{
  agg::trans_affine mtx;
  mtx.scale(mScale);
  mtx.translate(rect.L, rect.T);
  
  agg::rasterizer_scanline_aa<> pf;
  agg::scanline_p8 sl;
  
  agg::rect_i cb(0, 0, Width() * mScale, Height() * mScale);
  
  typedef agg::renderer_base<agg::pixfmt_gray8> mask_ren_base;
  agg::pixfmt_gray8 pixf(mAlphaMaskRenBuf);
  mask_ren_base maskRenBase(pixf);
  
  typedef agg::scanline_u8_am<agg::alpha_mask_gray8> scanline_type;
  agg::alpha_mask_gray8 mask(mAlphaMaskRenBuf);
  scanline_type msl(mask);
    
#ifdef WIN32
  agg::alpha_mask_rgba32gray img_mask_pixf;
#else
  agg::alpha_mask_argb32gray img_mask_pixf;
#endif
  
  pathRenderer.render(pf, sl, msl, mRenBase, maskRenBase, mPixf, img_mask_pixf, mtx, cb, *this);
  }


agg::pixel_map * IGraphicsAGG::load_image(const char* filename)
{
  IBitmap bitmap = LoadIBitmap(filename, 1, 1.0);
  return (agg::pixel_map *)bitmap.mData;
}

*/


void IGraphicsAGG::Draw(const IRECT& rect)
{
  mRenBase.clear(agg::rgba(1, 1, 1));
  IGraphics::Draw(rect);
}

void IGraphicsAGG::ToPixel(float & pixel)
{
  pixel = floorf(pixel + 0.5f) + 0.5f;
}
