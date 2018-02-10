#pragma once

/*

 AGG 2.4 should be modified to avoid bringing carbon headers on mac, which can cause conflicts

 in "agg_mac_pmap.h" ...
 //#include <ApplicationServices/ApplicationServices.h>
 #include <CoreGraphics/CoreGraphics.h>

 */

#include "IGraphics.h"
#include "IGraphicsAGG_src.h"

class AGGBitmap : public APIBitmap
{
public:
  AGGBitmap(agg::pixel_map* pPixMap, int scale) : APIBitmap (pPixMap, pPixMap->width(), pPixMap->height(), scale) {}
  virtual ~AGGBitmap() { delete ((agg::pixel_map*) GetBitmap()); }
};

inline const agg::rgba8 AGGColor(const IColor& color, const IBlend* pBlend = nullptr)
{
  return agg::rgba8(color.R, color.G, color.B, (BlendWeight(pBlend) * color.A));
}

inline agg::comp_op_e AGGBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
    return agg::comp_op_src;

  switch (pBlend->mMethod)
  {
    case kBlendClobber: return agg::comp_op_src_over;
    case kBlendAdd: return agg::comp_op_plus;
    case kBlendColorDodge: return agg::comp_op_color_dodge;
    case kBlendNone:
    default:
      return agg::comp_op_src_over;
  }
}

inline const agg::cover_type AGGCover(const IBlend* pBlend = nullptr)
{
  if (!pBlend)
    return 255;

  return std::max(agg::cover_type(0), std::min(agg::cover_type(roundf(pBlend->mWeight * 255.f)), agg::cover_type(255)));
}

/** IGraphics draw class using Antigrain Geometry
*   @ingroup DrawClasses*/
class IGraphicsAGG : public IGraphics
{
public:
  struct LineInfo
  {
    int mStartChar;
    int mEndChar;
    double mWidth;
    LineInfo() : mWidth(0.0), mStartChar(0), mEndChar(0) {}
  };

#ifdef OS_WIN
  typedef agg::order_bgra PixelOrder;
#else
  typedef agg::order_argb PixelOrder;
#endif
  typedef agg::comp_op_adaptor_rgba<agg::rgba8, PixelOrder> BlenderType;
  typedef agg::comp_op_adaptor_rgba_pre<agg::rgba8, PixelOrder> BlenderTypePre;
  typedef agg::pixfmt_custom_blend_rgba<BlenderType, agg::rendering_buffer> PixfmtType;
  typedef agg::pixfmt_custom_blend_rgba<BlenderTypePre, agg::rendering_buffer> PixfmtTypePre;
  typedef agg::renderer_base <PixfmtType> RenbaseType;
  typedef agg::font_engine_freetype_int32 FontEngineType;
  typedef agg::font_cache_manager <FontEngineType> FontManagerType;
  typedef agg::span_interpolator_linear<> interpolatorType;
  typedef agg::image_accessor_clip<PixfmtType> imgSourceType;
  typedef agg::span_image_filter_rgba_bilinear_clip <PixfmtType, interpolatorType> spanGenType;
  typedef agg::renderer_scanline_aa_solid<RenbaseType> rendererSolid;
  typedef agg::renderer_scanline_bin_solid<RenbaseType> rendererBin;
  typedef agg::renderer_base<agg::pixfmt_gray8> maskRenBase;
  typedef agg::scanline_u8_am<agg::alpha_mask_gray8> scanlineType;

  IGraphicsAGG(IGraphicsDelegate& dlg, int w, int h, int fps);
  ~IGraphicsAGG();

  void SetDisplayScale(int scale) override;

  void Draw(const IRECT& rect) override;

  void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend) override {}
  void DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend) override {}

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override;
  void ForcePixel(const IColor& color, int x, int y) override;

  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend) override;
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend) override;
  void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend = 0) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend) override;

  void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;

  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend) override;
  void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override;
  void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend) override;
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override;

  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure = false) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;

  IColor GetPoint(int x, int y) override;
  void* GetData() override { return 0; } //todo
  const char* GetDrawingAPIStr() override { return "AGG"; }

 // IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* cacheName, int scale) override;
 //  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override;

  void RenderDrawBitmap() override;

private:

  template <typename pathType>
  void Rasterize(const IColor& color, pathType& path, const IBlend* pBlend = nullptr)
  {
    agg::rasterizer_scanline_aa<> rasterizer;
    rasterizer.reset();

    agg::scanline_p8 scanline;

    agg::renderer_scanline_aa_solid<RenbaseType> renderer(mRenBase);

    renderer.color(AGGColor(color, pBlend));
    rasterizer.filling_rule(agg::fill_non_zero);

    rasterizer.add_path(path);
    agg::render_scanlines(rasterizer, scanline, renderer);
  }

  template <typename pathType>
  void Fill(const IColor& color, pathType& path, const IBlend* pBlend = nullptr)
  {
    Rasterize(color, path, pBlend);
  }

  template <typename pathType>
  void Stroke(const IColor& color, pathType& path, const IBlend* pBlend = nullptr)
  {
    agg::conv_stroke<pathType> strokes(path);
    strokes.width(1.0 * GetDisplayScale());
    Rasterize(color, strokes, pBlend);
  }

  RenbaseType mRenBase;
  PixfmtType mPixf;
  FontEngineType mFontEngine;
  FontManagerType mFontManager;
  agg::rendering_buffer mRenBuf;
#ifdef OS_MAC
  agg::pixel_map_mac mPixelMap;
#else
  //TODO:
#endif

private:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  agg::pixel_map* CreateAPIBitmap(int w, int h);
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int s) override;

  //pipeline to process the vectors glyph paths(curves + contour)
  agg::conv_curve<FontManagerType::path_adaptor_type> mFontCurves;
  agg::conv_contour<agg::conv_curve<FontManagerType::path_adaptor_type> > mFontContour;

  void CalculateTextLines(WDL_TypedBuf<LineInfo>* pLines, const IRECT& rect, const char* str, FontManagerType& manager);
  void ToPixel(float & pixel);
};
